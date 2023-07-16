/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "V4L2SourcePipeline"

#define MEAS_TIME 0				// 1フレーム当たりの描画時間を測定する時1

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	// #define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <cerrno>
#include <utility>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "utilbase.h"

#include "times.h"
#include "charutils.h"

// usb
#include "usb/descriptor_defs.h"
// v4l2
#include "v4l2/pipeline_v4l2_source.h"

namespace sere = serenegiant;
namespace serenegiant::v4l2::pipeline {

/**
 * 映像データ受け取り用バッファーの個数
 */
#define BUFFER_NUMS (4)
/**
 * デフォルトのv4l2ピクセルフォーマット
 * 0ならfind_streamで最初に見つかったピクセルフォーマットを使う
 * V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_UYVY
 * 実機だとV4L2_PIX_FMT_NV16が一番速そう(1フレームのサイズが小さいから？)
 */
#define DEFAULT_PIX_FMT (V4L2_PIX_FMT_NV16)	// (V4L2_PIX_FMT_MJPEG)
/**
 * 映像データ最大待ち時間
 */
#define MAX_WAIT_FRAME_US (100000L)

#if MEAS_TIME
#define MEAS_TIME_INIT	nsecs_t _meas_time_ = 0; int _meas_count_ = 0;
#define MEAS_TIME_START	const nsecs_t _meas_t_ = systemTime();
#define MEAS_TIME_STOP \
	_meas_time_ += (systemTime() - _meas_t_); \
	if UNLIKELY((++_meas_count_ % 100) == 0) { \
		const float d = _meas_time_ / (1000000.f * _meas_count_); \
		LOGI("draw time=%5.2f[msec]", d); \
	}
#else
#define MEAS_TIME_INIT
#define MEAS_TIME_START
#define MEAS_TIME_STOP
#endif

//--------------------------------------------------------------------------------
/**
 * コンストラクタ
 * @param device_name v4l2機器名
 */
/*public*/
V4L2SourcePipeline::V4L2SourcePipeline(std::string device_name)
:	sere_pipeline::IPipeline(),
	device_name(std::move(device_name)),
	m_fd(0), m_state(STATE_CLOSE),
	request_resize(false),
	request_pixel_format(DEFAULT_PIX_FMT),
	request_width(DEFAULT_PREVIEW_WIDTH), request_height(DEFAULT_PREVIEW_HEIGHT),
	request_min_fps(DEFAULT_PREVIEW_FPS_MIN), request_max_fps(DEFAULT_PREVIEW_FPS_MAX),
	stream_pixel_format(DEFAULT_PIX_FMT),
	stream_width(DEFAULT_PREVIEW_WIDTH), stream_height(DEFAULT_PREVIEW_HEIGHT), image_bytes(0),
	stream_frame_type(core::RAW_FRAME_UNKNOWN), stream_fps(0.0f),
	m_buffers(nullptr), m_buffersNums(0),
	v4l2_thread()
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
V4L2SourcePipeline::~V4L2SourcePipeline() {
	ENTER();

	set_state(sere_pipeline::PIPELINE_STATE_RELEASING);
	close();

	EXIT();
}

/**
 * コンストラクタで指定したv4l2機器をオープン
 * @return
 */
int V4L2SourcePipeline::open() {
	ENTER();

	int result = core::USB_ERROR_INVALID_STATE;

	AutoMutex lock(v4l2_lock);
	if (LIKELY(m_state == STATE_CLOSE)) {
		struct stat st{};
		int r = ::stat(device_name.c_str(), &st);
		if (LIKELY((r != -1) && S_ISCHR(st.st_mode))) {
			int fd = ::open(device_name.c_str(), O_RDWR /* required */| O_NONBLOCK, 0);
			if (LIKELY((fd >= 0))) {
				// オープンできた
				result = core::USB_SUCCESS;
				m_fd = fd;
				m_state = STATE_OPEN;
				update_ctrl_all_locked(fd, supported);
			} else {
				// ::openの返り値が負(-1)の時はオープンできていない
				result = -errno;
				LOGE("Cannot open '%s': %d, %s",
					device_name.c_str(), -result, strerror(-result));
			}
		} else {
			result = -errno;
			LOGE("Cannot identify '%s': r=%d, errno=%d/%s\n",
				device_name.c_str(), r, errno, strerror(errno));
		}
	} else {
		LOGD("Illegal state: already opened");
	}

	RETURN(result, int);
}

/**
 * ストリーム中であればストリーム終了したnoti
 * v4l2機器をオープンしていればクローズする
 * @return
 */
int V4L2SourcePipeline::close() {
	ENTER();

	int result = internal_stop();
	v4l2_lock.lock();
	{
		supported.clear();
		// v4l2機器をクローズ
		if (m_fd) {
			result = ::close(m_fd);
			if (UNLIKELY(result)) {
				LOGE("failed to close fd, result=%d,errno=%d", result, errno);
			}
			m_fd = 0;
		}
		m_state = STATE_CLOSE;
	}
	v4l2_lock.unlock();

	RETURN(result, int);
}

/**
 * コンストラクタで指定したv4l2機器をオープンして映像取得開始する
 * IPipelineの純粋仮想関数
 * @return
 */
/*public*/
int V4L2SourcePipeline::start() {
	ENTER();

	int result = core::USB_ERROR_INVALID_STATE;

	bool can_start;
	v4l2_lock.lock();
	{
		can_start = (m_state == STATE_OPEN) || (m_state == STATE_INIT);
	}
	v4l2_lock.unlock();

	if (can_start) {
		result = core::USB_SUCCESS;
		bool b = set_running(true);
		if (!b) {
			set_state(sere_pipeline::PIPELINE_STATE_STARTING);
			LOGD("映像取得用のワーカースレッドを開始");
			// 実際のイベントハンドラで初期化したthread型一時オブジェクトをムーブ代入
			v4l2_thread = std::thread([this] { v4l2_thread_func(); });
		}
	} else {
		LOGD("Illegal state: not opened");
	}

	RETURN(result, int);
}

/**
 * 映像取得終了
 * IPipelineの純粋仮想関数
 * @return
 */
/*public*/
int V4L2SourcePipeline::stop() {
	return internal_stop();
}

/**
 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
 * @return
 */
std::string V4L2SourcePipeline::get_supported_size() const {
	ENTER();

	std::vector<FormatInfoSp> formats;

	AutoMutex lock(v4l2_lock);
	if (m_state > STATE_CLOSE) {
		int r = 0;
		for (int i = 0 ; r != -1; i++) {
			struct v4l2_fmtdesc fmt {
				.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			};
			fmt.index = i;
			r = xioctl(m_fd, VIDIOC_ENUM_FMT, &fmt);
			if (r != -1) {
				LOGV("%i)%s(%s)", fmt.index,
					V4L2_PIX_FMT_to_string(fmt.pixelformat).c_str(),
					fmt.description);
				auto format = std::make_shared<format_info_t>(i, fmt.pixelformat);
				get_supported_frame_size(m_fd, format);
				formats.push_back(format);
			}
		}
	}
	// jsonへの出力
	char buf[128];
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	writer.StartObject();
	{
		writer.String(FORMATS);
		writer.StartArray();
		for (const auto &format: formats) {
			writer.StartObject();
			{
				write(writer, FORMAT_INDEX, format->index);
				write(writer, FRAME_TYPE, V4L2_PIX_FMT_to_raw_frame(format->pixel_format));
				write(writer, FRAME_DEFAULT, 1/*format->getDefaultFrameIndex()*/);	// 変な値が返ってくる時がある
				if (!format->frames.empty()) {
					writer.String(FRAME_SIZE);
					writer.StartArray();
					for (const auto& frame: format->frames) {
						snprintf(buf, sizeof(buf), "%dx%d", frame->width, frame->height);
						buf[sizeof(buf)-1] = '\0';
						writer.String(buf);
					}
					writer.EndArray();
					write_fps(writer, format->frames);	// detailキーにFPS配列を各解像度毎の配列として追加
				}
			}
			writer.EndObject();
		}

		writer.EndArray();
	}
	writer.EndObject();

	RET(std::string(buffer.GetString()));
}

/**
 * 指定したピクセルフォーマット・解像度・フレームレートに対応しているかどうかを確認
 * @param width
 * @param height
 * @param pixel_format V4L2_PIX_FMT_XXX, 省略時は前回に設定した値を使う
 * @param min_fps 最小フレームレート, 省略時はDEFAULT_PREVIEW_FPS_MINを使う
 * @param max_fps 最大フレームレート, 省略時はDEFAULT_PREVIEW_FPS_MAXを使う
 * @return 0: 対応している, 0以外: 対応していない
 */
int V4L2SourcePipeline::find_stream(
	const uint32_t &width, const uint32_t &height,
	uint32_t pixel_format,
	const float &min_fps, const float &max_fps) {

	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	if (m_state > STATE_CLOSE) {
		const uint32_t _pixel_format = pixel_format
			? pixel_format
			: (stream_pixel_format ? stream_pixel_format
				: (request_pixel_format ? request_pixel_format : DEFAULT_PIX_FMT));

		LOGD("target pixel format=0x%08x,sz(%dx%d)", _pixel_format, width, height);

		// デフォルトまたは前回のネゴシエーションでセットされている
		// ビデオキャプチャフォーマットを取得する
		struct v4l2_format capture_format{
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
		};

		result = xioctl(m_fd, VIDIOC_G_FMT, &capture_format);
		if (UNLIKELY(result == -1)) {
			LOGW("VIDIOC_G_FMT,errno=%d", errno);
			capture_format.fmt.pix.pixelformat = 0;
		}
		LOGD("default type=%d,sz(%dx%d),0x%08x=%s,field=%d",
			capture_format.type, capture_format.fmt.pix.width, capture_format.fmt.pix.height,
			capture_format.fmt.pix.pixelformat, V4L2_PIX_FMT_to_string(capture_format.fmt.pix.pixelformat).c_str(),
			capture_format.fmt.pix.field);

		int r = 0;
		for (int i = 0 ; result && (r != -1); i++) {
			struct v4l2_fmtdesc fmt {
				.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			};
			fmt.index = i;
			r = xioctl(m_fd, VIDIOC_ENUM_FMT, &fmt);
			if (r != -1) {
				const auto pxl_fmt = fmt.pixelformat;
				LOGD("%i)0x%08x=%s(%s)", fmt.index,
					pxl_fmt, V4L2_PIX_FMT_to_string(pxl_fmt).c_str(),
					fmt.description);
				if (!_pixel_format || (_pixel_format == pxl_fmt)) {
					// ピクセルフォーマットが一致したとき
					pixel_format = pxl_fmt;	// 最後に一致したピクセルフォーマットをセット
					LOGD("found pixel format, try find video size");
					result = find_frame_size(m_fd, pxl_fmt, width, height, min_fps, max_fps);
					// XXX VIDIOC_ENUM_FRAMESIZESが常にエラーを返してfind_frame_sizeで判断できないV4L2機器があるので
					//     デフォルトのキャプチャーフォーマットと一致すればOKとする
					if (result && (capture_format.fmt.pix.pixelformat == pxl_fmt)
						&& (capture_format.fmt.pix.width == width) && (capture_format.fmt.pix.height == height)) {
						result = 0;
					}
				}
				if (!result) {
					if (!request_pixel_format) {
						request_pixel_format = pxl_fmt;
					}
					if (!stream_pixel_format) {
						stream_pixel_format = pxl_fmt;
					}
					LOGD("%i)0x%08x=%s(%s),sz(%dx%d)", fmt.index,
						pxl_fmt, V4L2_PIX_FMT_to_string(pxl_fmt).c_str(),
						fmt.description, width, height);
				}
			}
		}

	}

	RETURN(result, int);
}

/**
 * 映像サイズを変更
 * @param width
 * @param height
 * @param pixel_format V4L2_PIX_FMT_XXX
 * @return
 */
/*public*/
int V4L2SourcePipeline::resize(
	const uint32_t &width, const uint32_t &height,
	const uint32_t &pixel_format) {

	ENTER();

	uint32_t format = pixel_format ? pixel_format : request_pixel_format;
	if ((request_width != width) || (request_height != height)
		|| (request_pixel_format != format)) {

		Mutex::Autolock lock(v4l2_lock);
		request_width = width;
		request_height = height;
		request_pixel_format = format;
		request_resize = true;
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * IPipelineの純粋仮想関数
 * @param frame
 * @return
 */
int V4L2SourcePipeline::queue_frame(core::BaseVideoFrame *frame) {
	return chain_frame(frame);
}

/**
 * 映像取得ループ開始時の処理, 映像取得スレッド上で実行される
 */
/*protected*/
void V4L2SourcePipeline::on_start() {
	ENTER();
	EXIT();
}

/**
 * 映像取得ループ終了時の処理, 映像取得スレッド上で実行される
 */
/*protected*/
void V4L2SourcePipeline::on_stop() {
	ENTER();
	EXIT();
}

/**
 * 映像処理スレッドの実行関数
 */
/*private*/
void V4L2SourcePipeline::v4l2_thread_func() {
	ENTER();

	int result;

	if (!is_running()) return;

	LOGD("映像処理スレッド開始");
	set_state(sere_pipeline::PIPELINE_STATE_RUNNING);
	on_start();
	v4l2_lock.lock();
	{
		LOGD("初期解像度・ピクセルフォーマットをセット");
		result = init_v4l2_locked(request_width, request_height, request_pixel_format);
		if (!result) {
			LOGD("映像ストリーム開始");
			result = start_stream_locked();
		}
	}
	v4l2_lock.unlock();

	LOGD("映像取得ループ,is_running=%d,result=%d", is_running(), result);
	for ( ; is_running() && !result; ) {
		v4l2_lock.lock();
		{
			if (request_resize) {
				request_resize = false;
				// 解像度変更処理
				result = handle_resize(request_width, request_height, request_pixel_format);
			}
		}
		v4l2_lock.unlock();
		// 映像取得ループへ
		if (!result) {
			v4l2_loop();
		}
	}	// for ( ; is_running() && !result; )

	MARK("映像取得ループ終了");

	// 終了処理
	v4l2_lock.lock();
	{
		stop_stream_locked();
		release_mmap_locked();
	}
	v4l2_lock.unlock();

	on_stop();

	EXIT();
}

/**
 * 映像取得ループ
 * @return
 */
/*private*/
int V4L2SourcePipeline::v4l2_loop() {
	ENTER();

	uvc::VideoFrame frame;
	frame.resize(stream_width, stream_height, stream_frame_type);
	frame.resize(image_bytes);

	// 実行中＆解像度・ピクセルフォーマット変更要求が無ければ映像取得する
	for ( ; is_running() && !request_resize; ) {
		// 映像フレームを待機
		int result = wait_frame(frame.frame(), image_bytes);
		if (result > 0) {
			// mjpegとかだと受信データサイズは固定では無いので
			// 実際のデータバイト数に合うようにリサイズする
			frame.resize(result);
			queue_frame(&frame);
		}
	} // for ( ; is_running(); )

	RETURN(core::USB_SUCCESS, int);
}

/**
 * ::stopの実体
 * ::stopはIPipelineの純粋仮想関数なのでデストラクタから呼び出すのは良くないので実際の処理内部関数として分離
 * @return
 */
/*private*/
int V4L2SourcePipeline::internal_stop() {
	ENTER();

	bool b = set_running(false);
	if (LIKELY(b)) {
		set_state(sere_pipeline::PIPELINE_STATE_STOPPING);
		if (v4l2_thread.joinable()) {
			LOGD("join:v4l2_thread");
			v4l2_thread.join();
		}
		set_state(sere_pipeline::PIPELINE_STATE_INITIALIZED);
		LOGD("v4l2_thread finished");
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * 映像ストリーム開始
 */
/*private*/
int V4L2SourcePipeline::start_stream_locked() {
	ENTER();

	LOGD("state=%d", m_state);

	int result = core::USB_ERROR_INVALID_STATE;

	if ((m_state == STATE_OPEN) || (m_state == STATE_INIT)) {
		// 予めすべてのバッファをキューに入れておく
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		for (uint32_t i = 0; i < m_buffersNums; ++i) {
			struct v4l2_buffer buf {
				.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
				.memory = V4L2_MEMORY_MMAP,
			};
			buf.index = i;
			if (xioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
				result = -errno;
				LOGE("VIDIOC_QBUF: errno=%d", -errno);
				goto ret;
			}
		}
		// ストリーム開始
		if (xioctl(m_fd, VIDIOC_STREAMON, &type) == -1) {
			result = -errno;
			LOGE("VIDIOC_STREAMON: errno=%d", -result);
			goto ret;
		}
		// success
		result = core::USB_SUCCESS;
	} else {
		LOGD("invalid state: state=%d", m_state);
	}

ret:
	RETURN(result, int);
}

/**
 * 映像ストリーム終了
 */
/*private*/
int V4L2SourcePipeline::stop_stream_locked() {
	ENTER();

	if (m_state == STATE_STREAM) {
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xioctl(m_fd, VIDIOC_STREAMOFF, &type) == -1) {
			LOGE("VIDIOC_STREAMOFF: errno=%d", errno);
		}
		m_state = STATE_INIT;
	} else {
		LOGD("Not streaming, state=%d", m_state);
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * 解像度とピクセルフォーマットをセットして映像データ受け取り用バッファーを初期化する
 * @param width
 * @param height
 * @return
 */
/*private*/
int V4L2SourcePipeline::init_v4l2_locked(
  	const uint32_t &width, const uint32_t &height,
  	const uint32_t &pixel_format) {

	ENTER();

	LOGD("sz(%dx%d)@%s/0x%08x", width, height, V4L2_PIX_FMT_to_string(pixel_format).c_str(), pixel_format);

	struct v4l2_capability cap{};

	int result = xioctl(m_fd, VIDIOC_QUERYCAP, &cap);
	if (UNLIKELY(result == -1)) {
		if (EINVAL == -errno) {
			LOGE("%s is not a V4L2 device", device_name.c_str());
		} else {
			LOGE("VIDIOC_QUERYCAP,errno=%d", errno);
		}
		RETURN(-errno, int);
	}
	if (UNLIKELY(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))) {
		LOGE("%s is not a video capture device", device_name.c_str());
		RETURN(core::USB_ERROR_NO_DEVICE, int);
	}
	if (UNLIKELY(!(cap.capabilities & V4L2_CAP_STREAMING))) {
		LOGE("%s does not support streaming i/o", device_name.c_str());
		RETURN(core::USB_ERROR_NOT_SUPPORTED, int);
	}

	// Select video input, video standard and tune here.
	struct v4l2_cropcap cropcap {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	};

	LOGD("VIDIOC_CROPCAP");
	result = xioctl(m_fd, VIDIOC_CROPCAP, &cropcap);
	if (!result) {
		struct v4l2_crop crop{};
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		LOGD("VIDIOC_S_CROP");
		if (xioctl(m_fd, VIDIOC_S_CROP, &crop) == -1) {
			if (errno == EINVAL) {
				LOGW("Cropping not supported.");
			} else {
				LOGD("VIDIOC_S_CROP:Errors ignored.,errno=%d", errno);
			}
		}
	} else {
		LOGD("VIDIOC_CROPCAP:Errors ignored.,errno=%d", errno);
	}

	struct v4l2_format fmt{};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = pixel_format;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	LOGD("VIDIOC_S_FMT");
	result = xioctl(m_fd, VIDIOC_S_FMT, &fmt);
	if (UNLIKELY(result == -1)) {
		LOGE("VIDIOC_S_FMT,errno=%d", errno);
		RETURN(core::USB_ERROR_NOT_SUPPORTED, int);
	}

	// Buggy driver paranoia.
	uint32_t min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min) {
		fmt.fmt.pix.bytesperline = min;
	}
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min) {
		fmt.fmt.pix.sizeimage = min;
	}

	// 画像データ読み込み用のメモリマップを初期化
	LOGD("call init_mmap_locked");
	result = init_mmap_locked();
	if (!result) {
		stream_width = fmt.fmt.pix.width;
		stream_height = fmt.fmt.pix.height;
		stream_pixel_format = fmt.fmt.pix.pixelformat;
		stream_frame_type = V4L2_PIX_FMT_to_raw_frame(stream_pixel_format);
		image_bytes = fmt.fmt.pix.sizeimage;
		request_resize = false;
	} else {
		release_mmap_locked();
	}

	RETURN(result, int);
}

/**
 * 映像データ受け取り用のメモリマップを開放
 */
/*private*/
int V4L2SourcePipeline::release_mmap_locked() {

	ENTER();

	if (m_buffersNums && m_buffers) {
		for (uint32_t i = 0; i < m_buffersNums; ++i) {
			if (m_buffers[i].start != MAP_FAILED) {
				if (munmap(m_buffers[i].start, m_buffers[i].length) == -1) {
					LOGE("munmap");
				}
			}
		}
		SAFE_DELETE_ARRAY(m_buffers);
		m_buffersNums = 0;
	}
	stream_width = stream_height = 0;
	if (m_state > STATE_OPEN) {
		m_state = STATE_OPEN;
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * 画像データ読み込み用のメモリマップを初期化
 * init_device_lockedの下請け
 */
/*private*/
int V4L2SourcePipeline::init_mmap_locked() {
	ENTER();

	int result = core::USB_SUCCESS;
	struct v4l2_requestbuffers req {
		.count = BUFFER_NUMS,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = V4L2_MEMORY_MMAP,
	};

	LOGD("VIDIOC_REQBUFS");
	if (xioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
		result = -errno;
		if (EINVAL == errno) {
			LOGE("%s does not support memory mapping", device_name.c_str());
		} else {
			LOGE("VIDIOC_REQBUFS,err=%d", result);
		}
		goto err;
	}

	if (req.count < BUFFER_NUMS) {
		LOGE("Insufficient buffer memory on %s", device_name.c_str());
		result = core::USB_ERROR_NO_MEM;
		goto err;
	}
	// 映像データ受け取り用バッファーを初期化
	m_buffers = new buffer_t[req.count];
	if (UNLIKELY(!m_buffers)) {
		result = core::USB_ERROR_NO_MEM;
		LOGE("Out of memory");
		goto err;
	}
	for (uint32_t i = 0; i < req.count; i++) {
		m_buffers[i].start = MAP_FAILED;
	}
	m_buffersNums = req.count;

	for (uint32_t i = 0; i < req.count; i++) {
		struct v4l2_buffer buf {
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.memory = V4L2_MEMORY_MMAP,
		};
		buf.index = i;

		LOGD("VIDIOC_QUERYBUF:%d", i);
		if (xioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1) {
			result = -errno;
			LOGE("VIDIOC_QUERYBUF,err=%d", result);
			break;
		}

		m_buffers[i].length = buf.length;
		m_buffers[i].start = mmap(nullptr /* start anywhere */, buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */, m_fd, (off_t)buf.m.offset);

		if (m_buffers[i].start == MAP_FAILED) {
			result = -errno;
			LOGE("mmap,err=%d", result);
			break;
		}
	}

err:
	if (result) {
		LOGD("something error occurred, release buffers,result=%d", result);
		SAFE_DELETE_ARRAY(m_buffers)
		m_buffersNums = 0;
	}
	RETURN(result, int);
}

/**
 * @brief 対応しているピクセルフォーマット一覧を取得する
 * 
 * @return std::vector 
 */
std::vector<uint32_t> V4L2SourcePipeline::get_supported_pixel_formats_locked(const std::vector<uint32_t> preffered) const {
	ENTER();

	std::vector<uint32_t> result;
	if (m_state > STATE_CLOSE) {
		int r = 0;
		for (int i = 0 ; (r != -1); i++) {
			struct v4l2_fmtdesc fmt {
				.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			};
			fmt.index = i;
			r = xioctl(m_fd, VIDIOC_ENUM_FMT, &fmt);
			if (r != -1) {
				const auto pixel_format = fmt.pixelformat;
				LOGD("%i)0x%08x=%s(%s)", fmt.index,
					pixel_format, V4L2_PIX_FMT_to_string(pixel_format).c_str(),
					fmt.description);
				if (preffered.empty() || (find(preffered.begin(), preffered.end(), pixel_format) != preffered.end())) {
					result.push_back(pixel_format);
				}				
			}
		}
	}
	LOGD("num pixel formast=%" FMT_SIZE_T, result.size());

	RET(result);
}

/**
 * 実際の解像度変更処理、ワーカースレッド上で実行される
 * @param width
 * @param height
 * @param pixel_format V4L2_PIX_FMT_XXX
 * @return
 */
/*private*/
int V4L2SourcePipeline::handle_resize(
	const uint32_t &width, const uint32_t &height,
	const uint32_t &pixel_format) {

	ENTER();
	LOGD("sz(%dx%d)@0x%08x", width, height, pixel_format);

	state_t prevState = m_state;
	int result = stop_stream_locked();	// state == STATE_INIT/STATE_OPEN
	if (LIKELY(!result)) {
		const uint32_t cur_width = stream_width ? stream_width : width;
		const uint32_t cur_height = stream_height ? stream_height : height;
		const uint32_t cur_pixel_format = stream_pixel_format ? stream_pixel_format : pixel_format;
		result = release_mmap_locked();	// state == STATE_OPEN
		if (LIKELY(!result)) {
			// 解像度・ピクセルフォーマットをセット
			result = init_v4l2_locked(width, height, pixel_format);
			if (result) {
				LOGD("以前の解像度・ピクセルフォーマットへ戻す,sz(%dx%d)@0x%08x",
					cur_width, cur_height, cur_pixel_format);
				result = init_v4l2_locked(cur_width, cur_height, cur_pixel_format);
			}
		}
		if (LIKELY(!result)) {
			m_state = STATE_INIT;
			switch (prevState) {
			case STATE_OPEN:	// 初回
			case STATE_INIT:	// 2回目以降
				break;			// 何もしない
			case STATE_STREAM:
				start_stream_locked();
				break;
			default:
				break;
			}
		}
	}

	RETURN(result, int);
}

/**
 * 映像データを取得する
 * 映像データがないときはブロックする
 * @param dst
 * @param capacity
 * @return 負:エラー 0以上:読み込んだデータバイト数
 */
/*private*/
int V4L2SourcePipeline::wait_frame(uint8_t *dst, const size_t &capacity) {

	ENTER();

	struct timeval tv {
		.tv_sec = 0,
	 	.tv_usec = MAX_WAIT_FRAME_US,
	};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_fd, &fds);
	/* Timeout. */
	// 映像データが準備できるまで待機
	int result = select(m_fd + 1, &fds, nullptr, nullptr, &tv);
	if (result < 0) {
		// selectがエラーを返した時
		result = -errno;
		if (EINTR != errno) {
			// 中断以外のエラーの場合はログ出力
			LOGE("select:errno=%d", -result);
		}
	} else {
		// 映像データの準備ができたかタイムアウトした時
		if (FD_ISSET(m_fd, &fds)) {
			// 映像データを読み込み
			result = read_frame(dst, capacity);
		} else {
			// EAGAIN(タイムアウト)
			result = 0;
		}
	}

	RETURN(result, int);
}

/**
 * 映像データを指定したバッファにコピーする
 * @param dst
 * @param capacity
 * @return 負:エラー, 0以上:読み込んだ映像データのバイト数
 */
/*private*/
int V4L2SourcePipeline::read_frame(uint8_t *dst, const size_t &capacity) {
	ENTER();

	struct v4l2_buffer buf{
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = V4L2_MEMORY_MMAP,
	};

	// 映像の入ったバッファを取得
	int result = xioctl(m_fd, VIDIOC_DQBUF, &buf);
	if (result >= 0) {
		// バッファを取得できた時
		if (buf.index < m_buffersNums) {
			if (LIKELY(dst && (capacity >= buf.bytesused))) {
				memcpy(dst, m_buffers[buf.index].start, buf.bytesused);
				// コピーしたサイズを返す
				result = (int)buf.bytesused;
			} else {
				result = 0;
			}
		}
		// 読み込み終わったバッファをキューに追加
		if (xioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
			result = -errno;
			LOGE("VIDIOC_QBUF: errno=%d", -result);
		}
	} else {
		result = -errno;
		switch (errno) {
		case EAGAIN:
			// 映像データが準備出来てない
			result = 0;
			break;
		case EIO:
			/* Could ignore EIO, see spec. */
			/* fall through */
		default:
			LOGE("VIDIOC_DQBUF: errno=%d", -result);
		}
	}

	RETURN(result, int);
}

/**
 * ctrl_idで指定したコントロール機能に対応しているかどうかを取得
 * v4l2機器をオープンしているときのみ有効。closeしているときは常にfalseを返す
 * @param ctrl_id
 * @return
 */
/*public*/
bool V4L2SourcePipeline::is_ctrl_supported(const uint32_t &ctrl_id) {
	AutoMutex lock(v4l2_lock);
	return supported.find(ctrl_id) != supported.end();
}

/**
 * ctrl_idで指定したコントロール機能のQueryCtrlSpを返す
 * 未対応のコントロール機能またはv4l2機器をオープンしていなければnullptrを返す
 * @param ctrl_id
 * @return
 */
/*protected*/
QueryCtrlSp V4L2SourcePipeline::get_ctrl(const uint32_t &ctrl_id) {
	AutoMutex lock(v4l2_lock);
	return supported.find(ctrl_id) != supported.end()
		? supported[ctrl_id] : nullptr;
}

/**
 * 最小値/最大値/ステップ値/デフォルト値/現在値を取得する
 * @param ctrl_id
 * @param values
 * @return
 */
/*public*/
int V4L2SourcePipeline::get_ctrl(
  	const uint32_t &ctrl_id,
  	uvc::control_value32_t &values) {

	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	auto query = supported.find(ctrl_id) != supported.end()
			? supported[ctrl_id] : nullptr;
	if (query) {
		values.id = query->id;
		values.min = query->minimum;
		values.max = query->maximum;
		values.step = query->step;
		values.def = query->default_value;
		struct v4l2_control ctrl {
			.id = ctrl_id,
		};
		result = xioctl(m_fd, VIDIOC_G_CTRL, &ctrl);
		if (!result) {
			values.current = ctrl.value;
		}
	}

	RETURN(result, int);
}

/**
 * ctrl_idで指定したコントロール機能の設定値を取得する
 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
 * @param ctrl_id
 * @param value
 * @return
 */
/*public*/
int V4L2SourcePipeline::get_ctrl_value(const uint32_t &ctrl_id, int32_t &value) {
	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	value = 0;
	if (supported.find(ctrl_id) != supported.end()) {
		struct v4l2_control ctrl {
			.id = ctrl_id,
		};
		result = xioctl(m_fd, VIDIOC_G_CTRL, &ctrl);
		if (!result) {
			value = ctrl.value;
		}
	}

	RETURN(result, int);
}

/**
 * ctrl_idで指定したコントロール機能へ値を設定する
 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
 * @param ctrl_id
 * @param value
 * @return
 */
/*public*/
int V4L2SourcePipeline::set_ctrl_value(const uint32_t &ctrl_id, const int32_t &value) {
	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	if (supported.find(ctrl_id) != supported.end()) {
		struct v4l2_control ctrl {
			.id = ctrl_id,
			.value = value,
		};
		result = xioctl(m_fd, VIDIOC_S_CTRL, &ctrl);
		if (result && (errno == ERANGE)) {
			// ドライバー側で値をクランプしたときは正常終了とする
			result = core::USB_SUCCESS;
		}
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
// v4l2標準コントロール
/**
 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_brightness(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_BRIGHTNESS, values);
}

/**
 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_brightness(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_BRIGHTNESS, value);
}

/**
 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
 * @return
 */
int32_t V4L2SourcePipeline::get_brightness() {
	int32_t result;
	get_ctrl_value(V4L2_CID_BRIGHTNESS, result);
	return result;
}

/**
 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_contrast(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_CONTRAST, values);
}

/**
 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_contrast(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_CONTRAST, value);
}

/**
 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
 * @return
 */
int32_t V4L2SourcePipeline::get_contrast() {
	int32_t result;
	get_ctrl_value(V4L2_CID_CONTRAST, result);
	return result;
}

/**
 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_saturation(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_SATURATION, values);
}

/**
 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_saturation(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_SATURATION, value);
}

/**
 *V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
 * @return
 */
int32_t V4L2SourcePipeline::get_saturation() {
	int32_t result;
	get_ctrl_value(V4L2_CID_SATURATION, result);
	return result;
}

/**
 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_hue(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_HUE, values);
}

/**
 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_hue(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_HUE, value);
}

/**
 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
 * @return
 */
int32_t V4L2SourcePipeline::get_hue() {
	int32_t result;
	get_ctrl_value(V4L2_CID_HUE, result);
	return result;
}

//#define V4L2_CID_AUDIO_VOLUME (V4L2_CID_BASE + 5)
//#define V4L2_CID_AUDIO_BALANCE (V4L2_CID_BASE + 6)
//#define V4L2_CID_AUDIO_BASS (V4L2_CID_BASE + 7)
//#define V4L2_CID_AUDIO_TREBLE (V4L2_CID_BASE + 8)
//#define V4L2_CID_AUDIO_MUTE (V4L2_CID_BASE + 9)
//#define V4L2_CID_AUDIO_LOUDNESS (V4L2_CID_BASE + 10)
//#define V4L2_CID_BLACK_LEVEL (V4L2_CID_BASE + 11)

/**
 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_auto_white_blance(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_AUTO_WHITE_BALANCE, values);
}

/**
 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_auto_white_blance(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_AUTO_WHITE_BALANCE, value);
}

/**
 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
 * @return
 */
int32_t V4L2SourcePipeline::get_auto_white_blance() {
	int32_t result;
	get_ctrl_value(V4L2_CID_AUTO_WHITE_BALANCE, result);
	return result;
}

//#define V4L2_CID_DO_WHITE_BALANCE (V4L2_CID_BASE + 13)
//#define V4L2_CID_RED_BALANCE (V4L2_CID_BASE + 14)
//#define V4L2_CID_BLUE_BALANCE (V4L2_CID_BASE + 15)

/**
 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_gamma(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_GAMMA, values);
}

/**
 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_gamma(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_GAMMA, value);
}

/**
 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
 * @return
 */
int32_t V4L2SourcePipeline::get_gamma() {
	int32_t result;
	get_ctrl_value(V4L2_CID_GAMMA, result);
	return result;
}


//#define V4L2_CID_WHITENESS (V4L2_CID_GAMMA)
//#define V4L2_CID_EXPOSURE (V4L2_CID_BASE + 17)
//#define

/**
 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_auto_gain(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_AUTOGAIN, values);
}

/**
 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_auto_gain(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_AUTOGAIN, value);
}

/**
 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
 * @return
 */
int32_t V4L2SourcePipeline::get_auto_gain() {
	int32_t result;
	get_ctrl_value(V4L2_CID_AUTOGAIN, result);
	return result;
}

/**
 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_gain(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_GAIN, values);
}

/**
 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_gain(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_GAIN, value);
}

/**
 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
 * @return
 */
int32_t V4L2SourcePipeline::get_gain() {
	int32_t result;
	get_ctrl_value(V4L2_CID_GAIN, result);
	return result;
}

//#define V4L2_CID_HFLIP (V4L2_CID_BASE + 20)
//#define V4L2_CID_VFLIP (V4L2_CID_BASE + 21)

/**
 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_powerline_frequency(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_POWER_LINE_FREQUENCY, values);
}

/**
 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_powerline_frequency(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_POWER_LINE_FREQUENCY, value);
}

/**
 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
 * @return
 */
int32_t V4L2SourcePipeline::get_powerline_frequency() {
	int32_t result;
	get_ctrl_value(V4L2_CID_POWER_LINE_FREQUENCY, result);
	return result;
}

/**
 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_auto_hue(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_HUE_AUTO, values);
}

/**
 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_auto_hue(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_HUE_AUTO, value);
}

/**
 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
 * @return
 */
int32_t V4L2SourcePipeline::get_auto_hue() {
	int32_t result;
	get_ctrl_value(V4L2_CID_HUE_AUTO, result);
	return result;
}

/**
 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_white_blance(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_WHITE_BALANCE_TEMPERATURE, values);
}

/**
 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_white_blance(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

/**
 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
 * @return
 */
int32_t V4L2SourcePipeline::get_white_blance() {
	int32_t result;
	get_ctrl_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE, result);
	return result;
}

/**
 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_sharpness(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_SHARPNESS, values);
}

/**
 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_sharpness(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_SHARPNESS, value);
}

/**
 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
 * @return
 */
int32_t V4L2SourcePipeline::get_sharpness() {
	int32_t result;
	get_ctrl_value(V4L2_CID_SHARPNESS, result);
	return result;
}

/**
 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_backlight_comp(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_BACKLIGHT_COMPENSATION, values);
}

/**
 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_backlight_comp(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_BACKLIGHT_COMPENSATION, value);
}

/**
 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
 * @return
 */
int32_t V4L2SourcePipeline::get_backlight_comp() {
	int32_t result;
	get_ctrl_value(V4L2_CID_BACKLIGHT_COMPENSATION, result);
	return result;
}

//#define V4L2_CID_CHROMA_AGC (V4L2_CID_BASE + 29)
//#define V4L2_CID_COLOR_KILLER (V4L2_CID_BASE + 30)
//#define V4L2_CID_COLORFX (V4L2_CID_BASE + 31)
//#define V4L2_CID_AUTOBRIGHTNESS (V4L2_CID_BASE + 32)
//#define V4L2_CID_BAND_STOP_FILTER (V4L2_CID_BASE + 33)
//#define V4L2_CID_ROTATE (V4L2_CID_BASE + 34)
//#define V4L2_CID_BG_COLOR (V4L2_CID_BASE + 35)
//#define V4L2_CID_CHROMA_GAIN (V4L2_CID_BASE + 36)
//#define V4L2_CID_ILLUMINATORS_1 (V4L2_CID_BASE + 37)
//#define V4L2_CID_ILLUMINATORS_2 (V4L2_CID_BASE + 38)
//#define V4L2_CID_MIN_BUFFERS_FOR_CAPTURE (V4L2_CID_BASE + 39)
//#define V4L2_CID_MIN_BUFFERS_FOR_OUTPUT (V4L2_CID_BASE + 40)
//#define V4L2_CID_ALPHA_COMPONENT (V4L2_CID_BASE + 41)
//#define V4L2_CID_COLORFX_CBCR (V4L2_CID_BASE + 42)

// v4l2標準カメラコントロール
/**
 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_exposure_auto(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_EXPOSURE_AUTO, values);
}

/**
 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_exposure_auto(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_EXPOSURE_AUTO, value);
}

/**
 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
 * @return
 */
int32_t V4L2SourcePipeline::get_exposure_auto() {
	int32_t result;
	get_ctrl_value(V4L2_CID_EXPOSURE_AUTO, result);
	return result;
}

/**
 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_exposure_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_EXPOSURE_ABSOLUTE, values);
}

/**
 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_exposure_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

/**
 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
 * @return
 */
int32_t V4L2SourcePipeline::get_exposure_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_EXPOSURE_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_exposure_auto_priority(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_EXPOSURE_AUTO_PRIORITY, values);
}

/**
 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_exposure_auto_priority(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_EXPOSURE_AUTO_PRIORITY, value);
}

/**
 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
 * @return
 */
int32_t V4L2SourcePipeline::get_exposure_auto_priority() {
	int32_t result;
	get_ctrl_value(V4L2_CID_EXPOSURE_AUTO_PRIORITY, result);
	return result;
}

/**
 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_pan_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_PAN_RELATIVE, values);
}

/**
 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_pan_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_PAN_RELATIVE, value);
}

/**
 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
 * @return
 */
int32_t V4L2SourcePipeline::get_pane_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_PAN_RELATIVE, result);
	return result;
}

/**
 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_tilt_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_TILT_RELATIVE, values);
}

/**
 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_tilt_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_TILT_RELATIVE, value);
}

/**
 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
 * @return
 */
int32_t V4L2SourcePipeline::get_tilt_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_TILT_RELATIVE, result);
	return result;
}

//#define V4L2_CID_PAN_RESET (V4L2_CID_CAMERA_CLASS_BASE + 6)
//#define V4L2_CID_TILT_RESET (V4L2_CID_CAMERA_CLASS_BASE + 7)

/**
 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_pan_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_PAN_ABSOLUTE, values);
}

/**
 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_pan_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_PAN_ABSOLUTE, value);
}

/**
 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
 * @return
 */
int32_t V4L2SourcePipeline::get_pan_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_PAN_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_tilt_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_TILT_ABSOLUTE, values);
}

/**
 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_tilt_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_TILT_ABSOLUTE, value);
}

/**
 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
 * @return
 */
int32_t V4L2SourcePipeline::get_tilt_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_TILT_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_focus_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_FOCUS_ABSOLUTE, values);
}

/**
 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_focus_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_FOCUS_ABSOLUTE, value);
}

/**
 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
 * @return
 */
int32_t V4L2SourcePipeline::get_focus_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_FOCUS_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_focus_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_FOCUS_RELATIVE, values);
}

/**
 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_focus_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_FOCUS_RELATIVE, value);
}

/**
 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
 * @return
 */
int32_t V4L2SourcePipeline::get_focus_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_FOCUS_RELATIVE, result);
	return result;
}

/**
 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_focus_auto(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_FOCUS_AUTO, values);
}

/**
 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_focus_auto(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_FOCUS_AUTO, value);
}

/**
 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
 * @return
 */
int32_t V4L2SourcePipeline::get_focus_auto() {
	int32_t result;
	get_ctrl_value(V4L2_CID_FOCUS_AUTO, result);
	return result;
}

/**
 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_zoom_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_ZOOM_ABSOLUTE, values);
}

/**
 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_zoom_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_ZOOM_ABSOLUTE, value);
}

/**
 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
 * @return
 */
int32_t V4L2SourcePipeline::get_zoom_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_ZOOM_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_zoom_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_ZOOM_RELATIVE, values);
}

/**
 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_zoom_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_ZOOM_RELATIVE, value);
}

/**
 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
 * @return
 */
int32_t V4L2SourcePipeline::get_zoom_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_ZOOM_RELATIVE, result);
	return result;
}

/**
 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_zoom_continuous(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_ZOOM_CONTINUOUS, values);
}

/**
 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_zoom_continuous(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_ZOOM_CONTINUOUS, value);
}

/**
 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
 * @return
 */
int32_t V4L2SourcePipeline::get_zoom_continuous() {
	int32_t result;
	get_ctrl_value(V4L2_CID_ZOOM_CONTINUOUS, result);
	return result;
}

/**
 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_privacy(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_PRIVACY, values);
}

/**
 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_privacy(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_PRIVACY, value);
}

/**
 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
 * @return
 */
int32_t V4L2SourcePipeline::get_privacy() {
	int32_t result;
	get_ctrl_value(V4L2_CID_PRIVACY, result);
	return result;
}

/**
 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_iris_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_IRIS_ABSOLUTE, values);
}

/**
 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_iris_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_IRIS_ABSOLUTE, value);
}

/**
 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
 * @return
 */
int32_t V4L2SourcePipeline::get_iris_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_IRIS_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
 * @param values
 * @return
 */
int V4L2SourcePipeline::update_iris_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_IRIS_RELATIVE, values);
}

/**
 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
 * @param value
 * @return
 */
int V4L2SourcePipeline::set_iris_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_IRIS_RELATIVE, value);
}

/**
 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
 * @return
 */
int32_t V4L2SourcePipeline::get_iris_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_IRIS_RELATIVE, result);
	return result;
}

//#define V4L2_CID_AUTO_EXPOSURE_BIAS (V4L2_CID_CAMERA_CLASS_BASE + 19)
//#define V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE (V4L2_CID_CAMERA_CLASS_BASE + 20)
//#define V4L2_CID_WIDE_DYNAMIC_RANGE (V4L2_CID_CAMERA_CLASS_BASE + 21)
//#define V4L2_CID_IMAGE_STABILIZATION (V4L2_CID_CAMERA_CLASS_BASE + 22)
//#define V4L2_CID_ISO_SENSITIVITY (V4L2_CID_CAMERA_CLASS_BASE + 23)
//#define V4L2_CID_ISO_SENSITIVITY_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 24)
//#define V4L2_CID_EXPOSURE_METERING (V4L2_CID_CAMERA_CLASS_BASE + 25)
//#define V4L2_CID_SCENE_MODE (V4L2_CID_CAMERA_CLASS_BASE + 26)
//#define V4L2_CID_3A_LOCK (V4L2_CID_CAMERA_CLASS_BASE + 27)
//#define V4L2_LOCK_EXPOSURE (1 << 0)
//#define V4L2_LOCK_WHITE_BALANCE (1 << 1)
//#define V4L2_LOCK_FOCUS (1 << 2)
//#define V4L2_CID_AUTO_FOCUS_START (V4L2_CID_CAMERA_CLASS_BASE + 28)
//#define V4L2_CID_AUTO_FOCUS_STOP (V4L2_CID_CAMERA_CLASS_BASE + 29)
//#define V4L2_CID_AUTO_FOCUS_STATUS (V4L2_CID_CAMERA_CLASS_BASE + 30)
//#define V4L2_AUTO_FOCUS_STATUS_IDLE (0 << 0)
//#define V4L2_AUTO_FOCUS_STATUS_BUSY (1 << 0)
//#define V4L2_AUTO_FOCUS_STATUS_REACHED (1 << 1)
//#define V4L2_AUTO_FOCUS_STATUS_FAILED (1 << 2)
//#define V4L2_CID_AUTO_FOCUS_RANGE (V4L2_CID_CAMERA_CLASS_BASE + 31)
//#define V4L2_CID_PAN_SPEED (V4L2_CID_CAMERA_CLASS_BASE + 32)
//#define V4L2_CID_TILT_SPEED (V4L2_CID_CAMERA_CLASS_BASE + 33)
//#define V4L2_CID_CAMERA_ORIENTATION (V4L2_CID_CAMERA_CLASS_BASE + 34)
//#define V4L2_CAMERA_ORIENTATION_FRONT 0
//#define V4L2_CAMERA_ORIENTATION_BACK 1
//#define V4L2_CAMERA_ORIENTATION_EXTERNAL 2
//#define V4L2_CID_CAMERA_SENSOR_ROTATION (V4L2_CID_CAMERA_CLASS_BASE + 35)

}	// namespace serenegiant::v4l2::pipeline

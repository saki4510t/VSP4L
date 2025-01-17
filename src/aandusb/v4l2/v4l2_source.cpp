/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "V4l2Source"

// 1フレーム当たりの描画時間を測定する時1
#define MEAS_TIME (0)
// 対応するコントロール機能をログへ出力するかどうか, 0: 出力しない, 1: 出力する
#define DUMP_SUPPRTED_CTRLS (0)

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
#include <cmath>
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
#include "v4l2/v4l2_source.h"

namespace serenegiant::v4l2 {

/**
 * デフォルトのv4l2ピクセルフォーマット
 * 0ならfind_streamで最初に見つかったピクセルフォーマットを使う
 * V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_UYVY
 * 実機だとV4L2_PIX_FMT_NV16が一番速そう(1フレームのサイズが小さいから？)
 */
#define DEFAULT_PIX_FMT (0) // (V4L2_PIX_FMT_NV16)	// (V4L2_PIX_FMT_MJPEG)

#if MEAS_TIME
#define MEAS_TIME_INIT static nsecs_t _meas_time_ = 0;\
   	static nsecs_t _init_time_ = systemTime();\
   	static int _meas_count_ = 0;
#define MEAS_TIME_START	const nsecs_t _meas_t_ = systemTime();
#define MEAS_TIME_STOP \
	_meas_time_ += (systemTime() - _meas_t_); \
	_meas_count_++; \
	if (UNLIKELY((_meas_count_ % 100) == 0)) { \
		const float d = _meas_time_ / (1000000.f * _meas_count_); \
		const float fps = _meas_count_ * 1000000000.f / (systemTime() - _init_time_); \
		LOGI("meas time=%5.2f[msec]/fps=%5.2f", d, fps); \
	}
#define MEAS_RESET \
	_meas_count_ = 0; \
	_meas_time_ = 0; \
	_init_time_ = systemTime();
#else
#define MEAS_TIME_INIT
#define MEAS_TIME_START
#define MEAS_TIME_STOP
#define MEAS_RESET
#endif

//--------------------------------------------------------------------------------
/**
 * コンストラクタ
 * @param device_name v4l2機器名
 * @param async 映像データの受取りを専用ワーカースレッドで行うかどうか
 */
/*public*/
V4l2SourceBase::V4l2SourceBase(
	std::string device_name,
	const bool &async,
	std::string udmabuf_name)
:	V4L2Ctrl(),
	device_name(std::move(device_name)), async(async),
	udmabuf_name(std::move(udmabuf_name)),
	m_running(false),
	m_fd(0), m_state(STATE_CLOSE), m_udmabuf_fd(0),
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
V4l2SourceBase::~V4l2SourceBase() {
	ENTER();

	close();

	EXIT();
}

/**
 * コンストラクタで指定したv4l2機器をオープン
 * @return
 */
int V4l2SourceBase::open() {
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
				update_ctrl_all_locked(fd, supported, DUMP_SUPPRTED_CTRLS);
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
int V4l2SourceBase::close() {
	ENTER();

	int result = internal_stop();
	v4l2_lock.lock();
	{
		supported.clear();
		m_state = STATE_CLOSE;
	}
	v4l2_lock.unlock();

	RETURN(result, int);
}

/**
 * コンストラクタで指定したv4l2機器をオープンして映像取得開始する
 * @param buf_num キャプチャ用のバッファ数、デフォルトはDEFAULT_BUFFER_NUMS
 * @return
 */
/*public*/
int V4l2SourceBase::start(const int &buf_nums) {
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
			LOGD("映像取得用のワーカースレッドを開始");
			// 実際のイベントハンドラで初期化したthread型一時オブジェクトをムーブ代入
			v4l2_thread = std::thread([this, buf_nums] { v4l2_thread_func(buf_nums); });
		}
	} else {
		LOGD("Illegal state: not opened");
	}

	RETURN(result, int);
}

/**
 * 映像取得終了
 * @return
 */
/*public*/
int V4l2SourceBase::stop() {
	return internal_stop();
}

/**
 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
 * @return
 */
/*public*/
std::string V4l2SourceBase::get_supported_size() const {
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
/*public*/
int V4l2SourceBase::find_stream(
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

		result = core::USB_ERROR_NOT_SUPPORTED;
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
					// ピクセルフォーマット未指定かピクセルフォーマットが一致したとき
					pixel_format = pxl_fmt;	// 最後に一致したピクセルフォーマットをセット
					LOGD("found pixel format, try find video size");
					result = find_frame_size(m_fd, pxl_fmt, width, height, min_fps, max_fps);
					// XXX VIDIOC_ENUM_FRAMESIZESが常にエラーを返してfind_frame_sizeで判断できないV4L2機器があるので
					//     デフォルトのキャプチャーフォーマットと一致すればOKとする
					if (result
						&& (capture_format.fmt.pix.width == width) && (capture_format.fmt.pix.height == height)
						&& ((capture_format.fmt.pix.pixelformat == pxl_fmt) || !get_frame_size_nums(m_fd, pxl_fmt))) {
						result = 0;
					}
				}
				if (!result) {
					if (!request_pixel_format) {
						LOGD("use 0x%08x/%s as request_pixel_format", pxl_fmt, V4L2_PIX_FMT_to_string(pxl_fmt).c_str());
						request_pixel_format = pxl_fmt;
					}
					if (!stream_pixel_format) {
						LOGD("use 0x%08x/%s as stream_pixel_format", pxl_fmt, V4L2_PIX_FMT_to_string(pxl_fmt).c_str());
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
int V4l2SourceBase::resize(
	const uint32_t &width, const uint32_t &height,
	const uint32_t &pixel_format) {

	ENTER();

	uint32_t format = pixel_format ? pixel_format : request_pixel_format;
	if ((request_width != width) || (request_height != height)
		|| (request_pixel_format != format)) {

		Mutex::Autolock lock(v4l2_lock);
		request_width = width;
		request_height = height;
		if (format) {
			request_pixel_format = format;
		}
		LOGD("request resize(%dx%d),format=0x%08x", width, height, request_pixel_format);
		request_resize = true;
	}

	RETURN(core::USB_SUCCESS, int);
}

//--------------------------------------------------------------------------------
/**
 * 映像処理スレッドの実行関数
 * @param buf_nums
 */
/*private*/
void V4l2SourceBase::v4l2_thread_func(const int &buf_nums) {
	ENTER();

	int result;

	if (!is_running()) return;

	LOGD("映像処理スレッド開始");
	on_start();
	v4l2_lock.lock();
	{
		LOGD("初期解像度・ピクセルフォーマットをセット");
		result = init_v4l2_locked(buf_nums, request_width, request_height, request_pixel_format);
		if (LIKELY(!result)) {
			LOGD("映像ストリーム開始");
			result = start_stream_locked();
		}
	}
	v4l2_lock.unlock();

	if (LIKELY(!result)) {
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

			if (UNLIKELY(result)) {
				release_mmap_locked();
				on_error();
				break;
			}

			// 映像取得ループへ
			if (!result && async) {
				v4l2_loop();
			}
		}	// for ( ; is_running() && !result; )

		MARK("映像取得ループ終了");
	} else {
		on_error();
	}

	// 終了処理
	v4l2_lock.lock();
	{
		stop_stream_locked();
		release_mmap_locked();
		// start_stream_lockedが失敗するとVIDIOC_QUERYBUFが呼ばれたまま
		// dequeueされないままになってしまうのでv4l2機器も含めてすべてcloseする
		internal_close_locked();
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
int V4l2SourceBase::v4l2_loop() {
	ENTER();

	// 実行中＆解像度・ピクセルフォーマット変更要求が無ければ映像取得する
	for ( ; is_running() && !request_resize; ) {
		// 映像フレームを待機
		handle_frame();
	} // for ( ; is_running(); )

	RETURN(core::USB_SUCCESS, int);
}

/**
 * ::stopの実体
 * @return
 */
/*private*/
int V4l2SourceBase::internal_stop() {
	ENTER();

	bool b = set_running(false);
	if (LIKELY(b)) {
		if (v4l2_thread.joinable()) {
			LOGD("join:v4l2_thread");
			v4l2_thread.join();
		}
		LOGD("v4l2_thread finished");
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * オープンしているudmabufやv4l2機器を閉じる
*/
/*private*/
int V4l2SourceBase::internal_close_locked() {
	ENTER();

	if (m_udmabuf_fd) {
		// udmabufを開いていれば閉じる
		const auto result = ::close(m_udmabuf_fd);
		if (UNLIKELY(result)) {
			LOGE("failed to close m_udmabuf_fd, result=%d,errno=%d", result, errno);
		}
		m_udmabuf_fd = 0;
	}
	if (m_fd) {
		// v4l2機器を開いていれば閉じる
		const auto result = ::close(m_fd);
		if (UNLIKELY(result)) {
			LOGE("failed to close fd, result=%d,errno=%d", result, errno);
		}
		m_fd = 0;
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * 映像ストリーム開始
 */
/*private*/
int V4l2SourceBase::start_stream_locked() {
	ENTER();

	LOGD("state=%d", m_state);

	int result = core::USB_ERROR_INVALID_STATE;

	if ((m_state == STATE_OPEN) || (m_state == STATE_INIT)) {
		// 予めすべてのバッファをキューに入れておく
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		const uint32_t memory = (m_udmabuf_fd ? V4L2_MEMORY_USERPTR : V4L2_MEMORY_MMAP);
		for (uint32_t i = 0; i < m_buffersNums; ++i) {
			struct v4l2_buffer buf {
				.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
				.memory = memory,
			};
			if (memory == V4L2_MEMORY_USERPTR) {
				// buffer must be casted to unsigned long type in order to assign it to V4L2 buffer
				buf.m.userptr = reinterpret_cast<unsigned long>(m_buffers[i].start);
				buf.length = m_buffers[i].length;
			}
			buf.index = i;
			if (xioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
				// キューに入れれなかったとき
				result = -errno;
				LOGE("VIDIOC_QBUF: errno=%d", -errno);
				release_mmap_locked();
				goto ret;
			}
		}
		// ストリーム開始
		if (xioctl(m_fd, VIDIOC_STREAMON, &type) == -1) {
			result = -errno;
			LOGE("VIDIOC_STREAMON: errno=%d", -result);
			release_mmap_locked();
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
int V4l2SourceBase::stop_stream_locked() {
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
 * @param buf_nums
 * @param width
 * @param height
 * @param pixel_format 
 * @return
 */
/*private*/
int V4l2SourceBase::init_v4l2_locked(
	const int &buf_nums,
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
	result = init_mmap_locked(buf_nums);
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
int V4l2SourceBase::release_mmap_locked() {

	ENTER();

	internal_release_mmap_buffers();
	stream_width = stream_height = 0;
	if (m_state > STATE_OPEN) {
		m_state = STATE_OPEN;
	}

	if (m_udmabuf_fd) {
		::close(m_udmabuf_fd);
		m_udmabuf_fd = 0;
	}

	RETURN(core::USB_SUCCESS, int);
}

/**
 * m_buffersの破棄処理
*/
void V4l2SourceBase::internal_release_mmap_buffers() {
	ENTER();

	if (m_buffersNums && m_buffers) {
		for (uint32_t i = 0; i < m_buffersNums; ++i) {
			if (m_buffers[i].start != MAP_FAILED) {
				if (munmap(m_buffers[i].start, m_buffers[i].length) == -1) {
					LOGE("munmap");
				}
			}
			if (m_buffers[i].fd) {
				::close(m_buffers[i].fd);
			}
		}
		SAFE_DELETE_ARRAY(m_buffers);
		m_buffersNums = 0;
	}

	EXIT();
}

/**
 * @brief ceil3
 * @details ceil num specifiy digit
 * @param num number
 * @param base ceil digit
 * @return int32_t result
 */
static int32_t ceil3(int32_t num, int32_t base) {
    double x = (double)(num) / (double)(base);
    double y = ceil(x) * (double)(base);
    return (int32_t)(y);
}

/**
 * 画像データ読み込み用のメモリマップを初期化
 * init_device_lockedの下請け
 * @param buff_nums
 */
/*private*/
int V4l2SourceBase::init_mmap_locked(const int &buf_nums) {
	ENTER();

	int result = core::USB_SUCCESS;
	int _buf_nums = buf_nums;
	if (_buf_nums < 1) {
		_buf_nums = 1;
	}

	uint32_t memory = V4L2_MEMORY_MMAP;
	if (!udmabuf_name.empty()) {
		LOGD("try to open %s", udmabuf_name.c_str());
		m_udmabuf_fd = ::open(udmabuf_name.c_str(), O_RDWR);
		if (m_udmabuf_fd > 0) {
			LOGD("use udmabuf");
			memory = V4L2_MEMORY_USERPTR;
		} else {
			LOGD("Failed to open %s", udmabuf_name.c_str());
			m_udmabuf_fd = 0;
		}
	}

	// 映像データ受取用のバッファを要求する
	struct v4l2_requestbuffers req {
		.count = DEFAULT_BUFFER_NUMS,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = memory,
	};
	for (int i = _buf_nums; i >= 1; i--) {
		LOGD("VIDIOC_REQBUFS %d", i);
		req.count = i;
		if (xioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
			result = -errno;
			req.count = 0;
			if (EINVAL == errno) {
				LOGE("%s does not support memory mapping", device_name.c_str());
				goto err;
			} else {
				LOGD("VIDIOC_REQBUFS,err=%d", result);
			}
		} else {
			break;
		}
	}

	if (!req.count) {
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
		m_buffers[i].fd = 0;
		m_buffers[i].start = MAP_FAILED;
		m_buffers[i].offset = 0;
		m_buffers[i].length = 0;
	}
	m_buffersNums = req.count;

	if (m_udmabuf_fd > 0) {
		result = init_mmap_locked_udmabuf(req);
	} else {	// if (m_udmabuf_fd > 0)
		result = init_mmap_udmabuf_other(req);
	}

err:
	if (result) {
		LOGD("something error occurred, release buffers,result=%d", result);
		internal_release_mmap_buffers();
	}

	RETURN(result, int);
}

/**
 * udmabufを使うとき
 * init_mmap_lockedの下請け
 * @param req
*/
int V4l2SourceBase::init_mmap_locked_udmabuf(struct v4l2_requestbuffers &req) {
	ENTER();

	int result = 0;
	// 
	LOGD("num=%d,capabilities=0x%08x,mem=%d", req.count, req.capabilities, req.memory);
	uint32_t image_size = 0;
	for (uint32_t i = 0; i < req.count; i++) {
		struct v4l2_buffer buf {
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.memory = req.memory,
		};
		buf.index = i;

		LOGD("VIDIOC_QUERYBUF:%d", i);
		if (xioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1) {
			result = -errno;
			LOGE("VIDIOC_QUERYBUF,err=%d", result);
			break;
		}

		image_size = MAX(image_size, buf.length);

		LOGD("buf:index=%d,type=%d,used=%d,flags=0x%08x,field=0x%08x,seq=%d,mem=%d,m=%lu,len=%d",
			buf.index, buf.type, buf.bytesused, buf.flags, buf.field, buf.sequence,
			buf.memory, buf.m.userptr, buf.length);
	}
	if (result || !image_size) {
		LOGE("Failed to get image size");
		RETURN(result, int);
	}
	// DMA転送できるようにページサイズの倍数になるように調整する
	const auto offset = ceil3(image_size, sysconf(_SC_PAGE_SIZE));
	LOGD("image_size=%d,offset=%d", image_size, offset);
	for (int i = 0; i < req.count; i++) {
		m_buffers[i].length = image_size;
		m_buffers[i].offset = i * offset;
		m_buffers[i].start = mmap(nullptr /* start anywhere */, image_size,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */, m_udmabuf_fd, (off_t)(i * offset));
		if (m_buffers[i].start == MAP_FAILED) {
			result = -errno;
			LOGE("mmap,err=%d", result);
			break;
		}
		m_buffers[i].fd = m_udmabuf_fd;
		// 物理メモリーを割り当てるためにゼロクリアする
		// memsetではだめらしい
		{
			auto word_ptr = (uint8_t *)m_buffers[i].start;
			for (int j = 0; j < offset; j++) {
				word_ptr[j] = 0;
			}
		}
	}

	RETURN(result, int);
}

/**
 * V4L2_MEMORY_DMABUFまたはV4L2_MEMORY_MMAPを使うとき
 * init_mmap_lockedの下請け
 * @param req
*/
int V4l2SourceBase::init_mmap_udmabuf_other(struct v4l2_requestbuffers &req) {
	ENTER();

	int result = 0;
	// カメラドライバー側でメモリーを確保するとき
	req.memory = (req.capabilities & V4L2_BUF_CAP_SUPPORTS_DMABUF)
		? V4L2_MEMORY_DMABUF : (req.capabilities & V4L2_BUF_CAP_SUPPORTS_MMAP ? V4L2_MEMORY_MMAP : 0);
	LOGD("num=%d,capabilities=0x%08x,mem=%d", req.count, req.capabilities, req.memory);

	for (uint32_t i = 0; i < req.count; i++) {
		struct v4l2_buffer buf {
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.memory = req.memory,
		};
		buf.index = i;

		LOGD("VIDIOC_QUERYBUF:%d", i);
		if (xioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1) {
			result = -errno;
			LOGE("VIDIOC_QUERYBUF,err=%d", result);
			break;
		}

		m_buffers[i].length = buf.length;
		m_buffers[i].offset = buf.m.offset;
		m_buffers[i].start = mmap(nullptr /* start anywhere */, buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */, m_fd, (off_t)buf.m.offset);

		if (m_buffers[i].start == MAP_FAILED) {
			result = -errno;
			LOGE("mmap,err=%d", result);
			break;
		}
	}

	RETURN(result, int);
}

/**
 * @brief 対応しているピクセルフォーマット一覧を取得する
 *
 * @return std::vector
 */
/*private*/
std::vector<uint32_t> V4l2SourceBase::get_supported_pixel_formats_locked(const std::vector<uint32_t> preffered) const {
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
int V4l2SourceBase::handle_resize(
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
			result = init_v4l2_locked(m_buffersNums, width, height, pixel_format);
			if (result) {
				LOGD("以前の解像度・ピクセルフォーマットへ戻す,sz(%dx%d)@0x%08x",
					cur_width, cur_height, cur_pixel_format);
				result = init_v4l2_locked(m_buffersNums, cur_width, cur_height, cur_pixel_format);
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
 * 映像データの処理
 * 映像データがないときはmax_wait_frame_usで指定した時間待機する
 * ワーカースレッド上で呼ばれる
 * @param max_wait_frame_us 最大映像待ち時間[マイクロ秒]
 * @return int 負:エラー 0以上:読み込んだデータバイト数
 */
/*private*/
int V4l2SourceBase::handle_frame(const suseconds_t &max_wait_frame_us) {

	ENTER();

	struct timeval tv {
		.tv_sec = 0,
	 	.tv_usec = max_wait_frame_us,
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
			// 映像データの処理
			const uint32_t memory = (m_udmabuf_fd ? V4L2_MEMORY_USERPTR : V4L2_MEMORY_MMAP);
			struct v4l2_buffer buf {
				.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
				.memory = memory,
			};

			// 映像の入ったバッファを取得
			int result = xioctl(m_fd, VIDIOC_DQBUF, &buf);
			if (result >= 0) {
				// バッファを取得できた時
				if (buf.index < m_buffersNums) {
					result = on_frame_ready(m_buffers[buf.index], buf.bytesused);
				}
				// 読み込み終わったバッファをキューに追加
				if (xioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
					result = -errno;
					LOGE("VIDIOC_QBUF: errno=%d", -result);
				}
			} else {
				// バッファを取得できなかったとき
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
		} else {
			// EAGAIN(タイムアウト)
			result = 0;
		}
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * ctrl_idで指定したコントロール機能に対応しているかどうかを取得
 * v4l2機器をオープンしているときのみ有効。closeしているときは常にfalseを返す
 * @param ctrl_id
 * @return
 */
/*public*/
bool V4l2SourceBase::is_ctrl_supported(const uint32_t &ctrl_id) {
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
QueryCtrlSp V4l2SourceBase::get_ctrl(const uint32_t &ctrl_id) {
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
int V4l2SourceBase::get_ctrl(
  	const uint32_t &ctrl_id,
  	uvc::control_value32_t &values) {

	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	auto query = supported.find(ctrl_id) != supported.end()
			? supported[ctrl_id] : nullptr;
	if (query) {
		values.id = query->id;
		values.type = query->type;
		values.min = query->minimum;
		values.max = query->maximum;
		values.step = query->step;
		values.def = query->default_value;
		// 現在の設定値を取得
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
int V4l2SourceBase::get_ctrl_value(const uint32_t &ctrl_id, int32_t &value) {
	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	value = 0;
	if (supported.find(ctrl_id) != supported.end()) {
		// 現在の設定値を取得
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
int V4l2SourceBase::set_ctrl_value(const uint32_t &ctrl_id, const int32_t &value) {
	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	if (supported.find(ctrl_id) != supported.end()) {
		// 指定した値を適用
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

/**
 * コントロール機能がメニュータイプの場合の設定項目値を取得する
 * @param id
 * @param items 設定項目値をセットするstd::vector<std::string>
 */
int V4l2SourceBase::get_menu_items(const uint32_t &ctrl_id, std::vector<std::string> &items) {
	ENTER();

	AutoMutex lock(v4l2_lock);
	int result = core::USB_ERROR_NOT_SUPPORTED;

	if (supported.find(ctrl_id) != supported.end()) {
		const auto query = supported[ctrl_id].get();
		if (query) {
			result = v4l2::get_menu_items(m_fd, *query, items);
		}
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 *
 * @param device_name v4l2機器名
 * @param async 映像データの受取りを専用ワーカースレッドで行うかどうか
 * @param udmabuf_name udmabufのデバイスファイル名
 */
/*public*/
V4l2Source::V4l2Source(
	std::string device_name,
	const bool &async,
	std::string udmabuf_name)
:	V4l2SourceBase(device_name, async, udmabuf_name),
	on_frame_ready_callbac(nullptr)
{
	ENTER();
	EXIT();
}

/**
 * @brief デストラクタ
 *
 */
/*public*/
V4l2Source::~V4l2Source() {
	ENTER();
	EXIT();
}

/**
 * 映像取得ループ開始時の処理, 映像取得スレッド上で実行される
 */
/*protected*/
void V4l2Source::on_start() {
	ENTER();

	if (on_start_callback) {
		on_start_callback();
	}

	EXIT();
}

/**
 * 映像取得ループ終了時の処理, 映像取得スレッド上で実行される
 */
/*protected*/
void V4l2Source::on_stop() {
	ENTER();

	if (on_stop_callback) {
		on_stop_callback();
	}

	EXIT();
}

/**
 * 映像でエラーが発生したときの処理, 映像取得スレッド上で実行される
 */
/*protected*/
void V4l2Source::on_error() {
	ENTER();

	if (on_error_callback) {
		on_error_callback();
	}

	EXIT();
}


/**
 * @brief 新しい映像を受け取ったときの処理, 純粋仮想関数
 *        ワーカースレッド上で呼ばれる
 *
 * @param buf 共有メモリー情報
 * @param bytes 映像データのサイズ
 * @return int 負:エラー 0以上:読み込んだデータバイト数
 */
int V4l2Source::on_frame_ready(const buffer_t &buf, const size_t &bytes) {
	ENTER();

    MEAS_TIME_INIT

	MEAS_TIME_START

	int result = -1;
	if (LIKELY(on_frame_ready_callbac)) {
		const auto image = (const uint8_t *)buf.start;
		result = on_frame_ready_callbac(image, bytes, buf);
	}

	MEAS_TIME_STOP

	RETURN(result, int);
}

}	// namespace serenegiant::v4l2

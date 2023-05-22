/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "VideoGLRenderer"

#define COUNT_FRAMES (0)
#define MEAS_TIME (0)			// 1フレーム当たりの描画時間を測定する時1
#define USE_PBO (false)			// テクスチャへの転送時にPBOを使うかどうか...PBOを使うほうが遅いみたい
#define USE_VBO (true)			// GLRendererでの矩形描画時にバッファオブジェクトを使うかどうか...これはあんまり変わらないけど使うほうが頂点座標を都度転送しないので少しは速いはず

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
//	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif

#include <cstring>	// memcpy

#include "utilbase.h"
#include "glutils.h"
// core
#include "core/video_gl_renderer.h"
// uvc
#include "uvc/aanduvc.h"

namespace serenegiant::core {

// mjepgをデコードするときの変換先raw_frame_t
// androidのハードウエアバッファーを使わなければ下記のどれでも描画可能
// ハードウエアバッファーを使う場合は正常に描画できないのも多い
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_RGBX
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_RGB565
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_XRGB
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_RGB
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_BGR
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_BGRX
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_XBGR
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_GRAY8
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_I420
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_YV12
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_NV12
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_NV21
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_422p
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_422sp
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_444p
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_YUYV
//#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_UYVY
#define MJPEG_DECODE_TARGET RAW_FRAME_UNCOMPRESSED_YUV_ANY	// これが最速

#if MEAS_TIME
    #define MEAS_TIME_INIT	static nsecs_t _meas_time_ = 0;\
    	static nsecs_t _init_time_ = systemTime();\
    	static int _meas_count_ = 0;
#define MEAS_TIME_START	const nsecs_t _meas_t_ = systemTime();
#define MEAS_TIME_STOP \
	_meas_time_ += (systemTime() - _meas_t_); \
	_meas_count_++; \
	if (UNLIKELY((_meas_count_ % 100) == 0)) { \
		const float d = _meas_time_ / (1000000.f * _meas_count_); \
		const float fps = _meas_count_ * 1000000000.f / (systemTime() - _init_time_); \
		LOGI("draw time=%5.2f[msec]/fps=%5.2f", d, fps); \
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

/**
 * コンストラクタ
 * @param gl_version
 * @param clear_color
 * @param enable_hw_buffer
 */
/*public*/
VideoGLRenderer::VideoGLRenderer(
	const int &gl_version,
	const uint32_t &clear_color,
	const bool &enable_hw_buffer)
:	gl_version(gl_version),
	has_gl_ext_yuv_target(gl::hasGLExtension("GL_EXT_YUV_target")),
	clear_color(clear_color),
	converter(DEFAULT_DCT_MODE),
    yuvtexture(nullptr),
	uvtexture(nullptr),
    renderer(nullptr),
	preview_frame_type(RAW_FRAME_UNKNOWN),
	mjpeg_decoded_frame_type(RAW_FRAME_UNKNOWN),
	mjpeg_decode_target(MJPEG_DECODE_TARGET),
	preview_width(0),
	preview_height(0),
    raw_frame_bytes(0),
#if USE_IMAGE_BUFFER
	imageBuffer(),
#endif
#if defined(__ANDROID__)
	is_hw_buffer_supported(enable_hw_buffer && init_hardware_buffer()),
	hwBuffer(nullptr), wrapped(nullptr),
#endif
    work(),
    work1()
{
	ENTER();

	set_mvp_matrix(nullptr);

	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
VideoGLRenderer::~VideoGLRenderer()
{
	ENTER();

	release_renderer();
#if USE_IMAGE_BUFFER
	imageBuffer.reset();
#endif

	EXIT();
}

/**
 * 関係するリソースを破棄する
 * @return
 */
/*public*/
int VideoGLRenderer::release() {
	ENTER();

	gl::clear_window(clear_color);
	release_renderer();

	RETURN(USB_SUCCESS, int);
}

/**
 * モデルビュー変換行列をセット
 * @param matrix nullptrなら単位行列になる,それ以外の場合はoffsetから16個以上確保すること
 * @param offset
 * @return
 */
/*public*/
int VideoGLRenderer::set_mvp_matrix(const GLfloat *matrix, const int &offset) {
	ENTER();

	if (matrix) {
		memcpy(mvp_matrix, &matrix[offset], sizeof(GLfloat) * 16);
	} else {
		gl::setIdentityMatrix(mvp_matrix);
	}

	RETURN(USB_SUCCESS, int);
}

/*private*/
void VideoGLRenderer::release_renderer(const bool &release_hw_buffer) {
	SAFE_DELETE(yuvtexture);
	SAFE_DELETE(uvtexture);
	SAFE_DELETE(renderer);
#if defined(__ANDROID__)
	if (release_hw_buffer) {
		SAFE_DELETE(wrapped);
		SAFE_DELETE(hwBuffer);
	}
#endif
#if USE_IMAGE_BUFFER
	imageBuffer.reset();
#endif
}

//--------------------------------------------------------------------------------
#include "gl/texture_vsh.h"
#include "gl/yuv2_fsh.h"
#include "gl/uyvy_fsh.h"
#include "gl/yuv444p_fsh.h"
#include "gl/yuv422p_fsh.h"
#include "gl/yuv422sp_fsh.h"
#include "gl/yuv420p_fsh.h"
#include "gl/yuv420p_yv12_fsh.h"
#include "gl/yuv420sp_nv21_fsh.h"
#include "gl/yuv420sp_nv12_ext_yuv_target.h"
#include "gl/yuv420sp_nv12_fsh.h"
#include "gl/yuv420sp_nv21_ext_yuv_target.h"
#include "gl/rgba_fsh.h"
#include "gl/argb_fsh.h"
#include "gl/rgb565_fsh.h"
#include "gl/rgb_fsh.h"
#include "gl/bgr_fsh.h"
#include "gl/gray8_fsh.h"
#include "gl/abgr_fsh.h"
#include "gl/bgra_fsh.h"

/**
 * 描画用のGLRendererを生成する
 * @param gl_version
 * @param frame_type
 * @param is_ext_yuv_target
 * @return
 */
static gl::GLRenderer *createGLRenderer(
	const int &gl_version,
	const raw_frame_t &frame_type,
	const bool &is_ext_yuv_target = false) {

	ENTER();

	MARK("create GLRenderer,gl_version=%d,frame_type=0x%08x",
		gl_version, frame_type);
	gl::GLRenderer *result = nullptr;
	switch (frame_type) {
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, yuy2_as_rgba_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuy2_as_rgba_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, uyvy_as_rgba_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, uyvy_as_rgba_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_444p:	// yuv444p(y->u->v)
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, yuv444p_as_lumi_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv444p_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_422p:	// yuv422p(y->u->v)
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, yuv422p_as_lumi_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv422p_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_422sp:	// yuv422sp(y->uv)
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, yuv422sp_as_lumi_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv422sp_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_I420:	// yuv420p(y->u->v)
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, yuv420p_as_lumi_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv420p_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_YV12:	// yuv420p(y->v->u)
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, yuv420p_yv12_as_lumi_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv420p_yv12_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21:
		if (gl_version >= 300) {
			if (!is_ext_yuv_target) {
				LOGD("create GLRenderer GLES3");
				result = new gl::GLRenderer(texture_gl3_vsh, yuv420sp_nv21_as_lumi_gl3_fsh, USE_VBO);
			} else {
				LOGD("create GLRenderer GLES3,ext_yuv_target");
				result = new gl::GLRenderer(texture_gl3_vsh, yuv420sp_nv21_ext_yuv_target_gl3_fsh, USE_VBO);
			}
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv420sp_nv21_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_NV12:
		if (gl_version >= 300) {
			if (!is_ext_yuv_target) {
				LOGD("create GLRenderer GLES3");
				result = new gl::GLRenderer(texture_gl3_vsh, yuv420sp_nv12_as_lumi_gl3_fsh, USE_VBO);
			} else {
				LOGD("create GLRenderer GLES3,ext_yuv_target");
				result = new gl::GLRenderer(texture_gl3_vsh, yuv420sp_nv12_ext_yuv_target_gl3_fsh, USE_VBO);
			}
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, yuv420sp_nv12_as_lumi_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, rgba_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, rgba_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_XRGB:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, argb_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, argb_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, rgba_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, rgba_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, rgb_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, rgb_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, bgr_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, bgr_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, gray8_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, gray8_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_BGRX:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, bgra_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, bgra_gl2_fsh, USE_VBO);
		}
		break;
	case RAW_FRAME_UNCOMPRESSED_XBGR:
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			result = new gl::GLRenderer(texture_gl3_vsh, abgr_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			result = new gl::GLRenderer(texture_gl2_vsh, abgr_gl2_fsh, USE_VBO);
		}
		break;
	default:
		LOGW("Unsupported frame type,0x%08x", frame_type);
		break;
	}

	RET(result);
}

/**
 * 描画処理
 * イベントループから定期的に呼び出される
 */
/*public*/
int VideoGLRenderer::draw_frame(IVideoFrame &frame)
{
	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
    static int err_cnt = 0;
#endif
	if (UNLIKELY((preview_width != frame.width())
		|| (preview_height != frame.height())
		|| (preview_frame_type != frame.frame_type()) )) {
		// 以前のフレームサイズと今回のフレームサイズが異なるかフレームタイプが異なる場合
		LOGD("frame size and/or type changed! size:%dx%d=>%dx%d,tyep:%08x=>%08x",
			preview_width, preview_height, frame.width(), frame.height(),
			preview_frame_type, frame.frame_type());
		preview_width =(int)frame.width();
		preview_height = (int)frame.height();
		preview_frame_type = frame.frame_type();
		mjpeg_decoded_frame_type = RAW_FRAME_UNKNOWN;
		mjpeg_decode_target = MJPEG_DECODE_TARGET;
		release_renderer();
		raw_frame_bytes = get_pixel_bytes(preview_frame_type)
			.frame_bytes(preview_width, preview_height);
		LOGD("raw_frame_bytes %" FMT_SIZE_T "=>%d",
			raw_frame_bytes, raw_frame_bytes);
	}
	int result = on_draw_frame(frame);
#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
	if (result) {
		err_cnt++;
	}
	if ((++cnt % 100) == 0) {
		MARK("cnt=%d,err=%d", cnt, err_cnt);
	}
#endif

	RETURN(result, int);
}

/**
 * 映像の描画処理
 * @param frame
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_frame(IVideoFrame &frame) {
	ENTER();

	int result;
	switch (frame.frame_type()) {
	case RAW_FRAME_MJPEG:
#if defined(__ANDROID__)
		if (is_hw_buffer_supported) {
//			mjpeg_decode_target = RAW_FRAME_UNCOMPRESSED_NV21;
			result = on_draw_mjpeg_hw_buffer(frame);
		} else {
			result = on_draw_mjpeg(frame);
		}
#else
		result = on_draw_mjpeg(frame);
#endif
		break;
	default:
		result = on_draw_uncompressed(frame);
		break;
	}

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * MJPEG以外の分岐用
 * @param frame
 */
int VideoGLRenderer::on_draw_uncompressed(IVideoFrame &frame) {
	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;
	switch (frame.frame_type()) {
	case RAW_FRAME_UNCOMPRESSED_YUYV:
		result = on_draw_yuyv(frame);	// YUV2,インターリーブ
		break;
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		result = on_draw_uyvy(frame);	// UYVY,インターリーブ
		break;
	case RAW_FRAME_UNCOMPRESSED_YCbCr:
		result = on_draw_ycbcr(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_444p:	// yuv444p(y->u->v)
		result = on_draw_yuv444p(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_422p:	// yuv422p(y->u->v)
		result = on_draw_yuv422p(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_422sp:	// yuv422p(y->uv)
		result = on_draw_yuv422sp(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_I420:	// yuv420p(y->u->v)
		result = on_draw_yuv420p(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_YV12:	// yuv420p(y->v->u)
		result = on_draw_yuv420p_yv12(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21:
		result = on_draw_yuv420sp_nv21(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_NV12:
		result = on_draw_yuv420sp_nv12(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:
		result = on_draw_rgbx(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_XRGB:
		result = on_draw_xrgb(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB565:
		result = on_draw_rgb565(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB:
		result = on_draw_rgb(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_BGR:
		result = on_draw_bgr(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
		result = on_draw_gray8(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_BGRX:
		result = on_draw_bgrx(frame);
		break;
	case RAW_FRAME_UNCOMPRESSED_XBGR:
		result = on_draw_xbgr(frame);
		break;
	default:
		gl::clear_window(clear_color);
		break;
	}

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_MJPEGの時
 * mjpegのフレームデータをyuv展開してテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_mjpeg
 * @return recycleの必要なVideoFrameオブジェクト, nullptr可
 */
/*private*/
int VideoGLRenderer::on_draw_mjpeg(IVideoFrame &frame_mjpeg) {
	ENTER();

    MEAS_TIME_INIT

	MEAS_TIME_START
	// MJPEGからMJPEGのサブサンプリングに応じたYUVフォーマットへ変換してそのまま描画する場合
	// こちらはMJPEGの展開以外のフォーマット変換処理が無いので負荷低減・高速化が期待される
	// MJPEGをデコード
#if 1
	int result = converter.copy_to(frame_mjpeg, work, MJPEG_DECODE_TARGET);
#elif 0
    // こっちはVideoConverterのテスト用に1回余分に変換する
	int result = converter.copy_to(frame_mjpeg, work1, MJPEG_DECODE_TARGET);
	if (LIKELY(!result)) {
		result = converter.copy_to(work1, work, RAW_FRAME_UNCOMPRESSED_XRGB);
	}
#else
    // こっちはVideoImage_tを使ったVideoConverterのテスト用
    // OK: mjpeg(422p) -> 422p, NV21, NV12, 444p, YV12, I420, XRGB, XBGR, RGBX, BGRX, RGB, BGR, RGB565, YUYV, UYVY
    VideoImage_t image{};
    work.set_format(frame_mjpeg.width(), frame_mjpeg.height(), RAW_FRAME_UNCOMPRESSED_UYVY);
    work.get_image(image);
	int result = converter.copy_to(frame_mjpeg, image);
#endif
	if (LIKELY(!result)) {
		// デコードに成功した時
		if (UNLIKELY(mjpeg_decoded_frame_type != work.frame_type())) {
			MARK("mjpeg_decoded_frame_type changed!");
			release_renderer();
			mjpeg_decoded_frame_type = work.frame_type();
			raw_frame_bytes = get_pixel_bytes(mjpeg_decoded_frame_type).frame_bytes(preview_width, preview_height);
			MEAS_RESET
		}
		result = on_draw_uncompressed(work);
	} else {
		LOGW("Failed to decode mjpeg,err=%d", result);
	}

	MEAS_TIME_STOP

	RETURN(result, int);
}

#if defined(__ANDROID__)

#if USE_IMAGE_BUFFER
// mjpegからImageBufferへ展開した後ワーク用のBaseVideoFrameへ書き戻して表示する
// mjpeg(yuv422p) -> NV22, YV12, I420は正常に表示される

/*private*/
int VideoGLRenderer::on_draw_mjpeg_hw_buffer(IVideoFrame &frame_mjpeg) {
	ENTER();

    MEAS_TIME_INIT

	MEAS_TIME_START

	int result = USB_ERROR_NOT_SUPPORTED;
	raw_frame_t frame_type = mjpeg_decoded_frame_type;
	if (UNLIKELY(frame_type == RAW_FRAME_UNKNOWN)) {
		frame_type = converter.get_mjpeg_decode_type(frame_mjpeg);
		mjpeg_decoded_frame_type = frame_type;
		raw_frame_bytes = get_pixel_bytes(mjpeg_decoded_frame_type).frame_bytes(preview_width, preview_height);
		if (mjpeg_decode_target == RAW_FRAME_UNCOMPRESSED_YUV_ANY) {
			// ANYで初期化してしまうと実際のプレーンサイズと違って正常に描画できないことがあるので
			// ANYの時はMJPEGのデコード先のフレームタイプを実際のフレームタイプにする
			mjpeg_decode_target = mjpeg_decoded_frame_type;
		}
		MARK("frame_type=0x%08x,frame bytes=%" FMT_SIZE_T, frame_type, raw_frame_bytes);
		MEAS_RESET
	}
	if (!imageBuffer || (imageBuffer->width() != preview_width) || (imageBuffer->height() != preview_height)) {
		MARK("create ImageBuffer,hw_buffer=%d", is_hw_buffer_supported);
		imageBuffer = std::make_shared<ImageBuffer>(mjpeg_decode_target, preview_width, preview_height, is_hw_buffer_supported);
	}
	if (LIKELY(imageBuffer)) {
		// MJPEGからMJPEGのサブサンプリングに応じたYUVフォーマットへ変換してそのまま描画する場合
		// こちらはMJPEGの展開以外のフォーマット変換処理が無いので負荷低減・高速化が期待される
		// MJPEGをデコード
		auto image = imageBuffer->lock();
		if (image.frame_type != RAW_FRAME_UNKNOWN) {
			LOGV("extract mjpeg");
			result = converter.copy_to(frame_mjpeg, image);
			imageBuffer->unlock();
		} else {
			result = USB_ERROR_NOT_SUPPORTED;
			LOGD("failed to lock ImageBuffer");
		}
		if (LIKELY(!result)) {
			// 描画用にImageBufferからworkへ書き戻す
			image = imageBuffer->lock();
			if (image.frame_type != RAW_FRAME_UNKNOWN) {
				LOGV("copy to work");
				work.resize(imageBuffer->width(), imageBuffer->height(), imageBuffer->frame_type());
				VideoImage_t dst;
				std::vector<uint8_t> buffer;
				work.get_image(dst);
				result = copy(image, dst, buffer, false);
				imageBuffer->unlock();
				if (UNLIKELY(result)) {
					LOGW("failed to copy,err=%d,frame_type=0x%08x->0x%08x",
						result, imageBuffer->frame_type(), work.frame_type());
				}
			} else {
				result = USB_ERROR_NOT_SUPPORTED;
				LOGD("failed to lock ImageBuffer");
			}
		}
		if (LIKELY(!result)) {
			if (UNLIKELY(mjpeg_decoded_frame_type != work.frame_type())) {
				MARK("mjpeg_decoded_frame_type changed!,0x%08x,actual_bytes=%" FMT_SIZE_T, work.frame_type(), work.actual_bytes());
				release_renderer(false);
				mjpeg_decoded_frame_type = work.frame_type();
				raw_frame_bytes = get_pixel_bytes(mjpeg_decoded_frame_type).frame_bytes(preview_width, preview_height);
			}
			result = on_draw_frame(work);
		} else {
			LOGW("Failed to decode mjpeg,err=%d", result);
		}
	}

	if (result) {
		LOGD("処理できなかったときはフォールバックする");
		release_renderer();
		imageBuffer.reset();
		is_hw_buffer_supported = false;
		result = on_draw_mjpeg(frame_mjpeg);
	}

	MEAS_TIME_STOP

	RETURN(result, int);
}

#else	// #if USE_IMAGE_BUFFER
/**
 * on_drawの下請け
 * ハードウエアバッファに対応している場合
 * preview_frame_type == RAW_FRAME_MJPEGの時
 * XXX ハードウエアバッファーをEGLClientBuffer, eglCreateImageKHR,
 *       glEGLImageTargetTexture2DOESと組み合わせてテクスチャにすると
 *       1枚のテクスチャになってしまうが、セミプラナー形式のフラグメントシェーダーの
 *       多くがyとuvの2枚前提なのでうまく動かないみたい
 *       GLES3以降でGPUとそのドライバーがGL_EXT_YUV_target拡張に対応している場合
 *       フラグメントシェーダーで#extension GL_EXT_YUV_target : requireを
 *       宣言して、サンプラーに__samplerExternal2DY2YEXTを使うと
 *       YUVとして読み取れるようになるみたい。
 * 		 ※ yuv_2_rgbはビルトインのYUVからRGBへの変換関数
 * 		 	#version 300 es
 *			#extension GL_EXT_YUV_target : require
 *			#extension GL_OES_EGL_image_external_essl3 : require
 *			precision mediump float;
 *			uniform __samplerExternal2DY2YEXT uTexture;
 *			in vec2 vTexCoords;
 *			out vec4 outColor;
 * 			void main() {
 * 				vec3 srcYuv = texture(uTexture, vTexCoords).xyz;
 * 				outColor = vec4(yuv_2_rgb(srcYuv, itu_601), 1.0);
 * 			}
 * 		もしくは__samplerExternal2DY2YEXTの行を
 * 			uniform samplerExternalOES yuvTexSampler;
 * 		テクスチャ読み取り時に変換せずに
 * 			outColor = texture(yuvTexSampler, yuvTexCoords);
 * 		デフォルトの変換関数で表示される？らしい
 * @param frame_mjpeg
 * @return 0: 描画成功, それ以外: エラー
 */
/*private*/
int VideoGLRenderer::on_draw_mjpeg_hw_buffer(IVideoFrame &frame_mjpeg) {
	ENTER();

    MEAS_TIME_INIT

	MEAS_TIME_START

	int result = USB_ERROR_NOT_SUPPORTED;
	raw_frame_t frame_type = mjpeg_decoded_frame_type;
	if (UNLIKELY(frame_type == RAW_FRAME_UNKNOWN)) {
		frame_type = converter.get_mjpeg_decode_type(frame_mjpeg);
		mjpeg_decoded_frame_type = frame_type;
		raw_frame_bytes = get_pixel_bytes(mjpeg_decoded_frame_type)
			.frame_bytes(preview_width, preview_height);
		MARK("frame_type=0x%08x,frame bytes=%" FMT_SIZE_T, frame_type, raw_frame_bytes);
		MEAS_RESET
	}
	if (UNLIKELY(!hwBuffer)) {
		GLenum tex_target = GL_TEXTURE_2D;
		switch (mjpeg_decode_target) {
		case RAW_FRAME_UNCOMPRESSED_NV12:
		case RAW_FRAME_UNCOMPRESSED_NV21:
			if (has_gl_ext_yuv_target) {
				tex_target = GL_TEXTURE_EXTERNAL_OES;
			}
			// pass through
//		case RAW_FRAME_UNCOMPRESSED_I420:
//		case RAW_FRAME_UNCOMPRESSED_YV12:
//		case RAW_FRAME_UNCOMPRESSED_422p:
////	case RAW_FRAME_UNCOMPRESSED_422sp:
//		case RAW_FRAME_UNCOMPRESSED_444p:
//		case RAW_FRAME_UNCOMPRESSED_GRAY8:
//		case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
			MARK("create HWBufferVideoFrame");
			if (mjpeg_decode_target == RAW_FRAME_UNCOMPRESSED_YUV_ANY) {
				// ANYで初期化してしまうと1ピクセル4バイトで初期化してしまうので
				// 実際のプレーンサイズと違って正常に描画できないことがあるので
				// ANYの時はMJPEGのデコード先のフレームタイプを実際のフレームタイプにする
				mjpeg_decode_target = mjpeg_decoded_frame_type;
			}
			hwBuffer = new HWBufferVideoFrame(
				preview_width, preview_height,
				mjpeg_decode_target);
			if (hwBuffer->is_supported()) {
				MARK("wrap as GLTexture");
				wrapped = gl::GLTexture::wrap(
					hwBuffer->get_hardware_buffer(),
					tex_target, GL_TEXTURE0,
					preview_width, preview_height);
			} else {
				// 未対応の場合
				SAFE_DELETE(hwBuffer);
			}
			break;
		default:
			break;
		}
	}
	if (LIKELY(hwBuffer)) {
		// MJPEGからMJPEGのサブサンプリングに応じたYUVフォーマットへ変換してそのまま描画する場合
		// こちらはMJPEGの展開以外のフォーマット変換処理が無いので負荷低減・高速化が期待される
		// MJPEGをデコード
#if 0
		// こっちだとNV21/NV12のセミプレーンのハードウエアバッファーを
		// 直接GL_TEXTURE_EXTERNAL_OESのテクスチャに割り当てたときに
		// (YプレーンとUVプレーンが連続していないので)映像がずれてしまう
		hwBuffer->lock();
		{	// テクスチャのバックバッファのハードウエアバッファ上へ直接mjpegを展開する
			result = converter.copy_to(frame_mjpeg, *hwBuffer, mjpeg_decode_target);
		}
		hwBuffer->unlock();
#else
		VideoImage_t image;
		result = hwBuffer->lock(image);
		if (!result) {
			result = converter.copy_to(frame_mjpeg, image);
			hwBuffer->unlock();
		}
//		static int lock_cnt = 0;
//		if (((lock_cnt++) % 300) == 0) {
//			dump(image);
//		}
#endif
		if (LIKELY(!result)) {
			// デコードに成功した時
			if (UNLIKELY(mjpeg_decoded_frame_type != hwBuffer->frame_type())) {
				MARK("mjpeg_decoded_frame_type changed!,0x%08x,actual_bytes=%" FMT_SIZE_T,
					hwBuffer->frame_type(), hwBuffer->actual_bytes());
				release_renderer(false);
				mjpeg_decoded_frame_type = hwBuffer->frame_type();
				raw_frame_bytes = get_pixel_bytes(mjpeg_decoded_frame_type)
					.frame_bytes(preview_width, preview_height);
			}
			// hwBufferのテクスチャを描画する
			if (UNLIKELY(!renderer)) {
				switch (hwBuffer->frame_type()) {
//				case RAW_FRAME_UNCOMPRESSED_I420:
//				case RAW_FRAME_UNCOMPRESSED_YV12:
				case RAW_FRAME_UNCOMPRESSED_NV12:
				case RAW_FRAME_UNCOMPRESSED_NV21:
//				case RAW_FRAME_UNCOMPRESSED_422p:
////				case RAW_FRAME_UNCOMPRESSED_422sp:
//				case RAW_FRAME_UNCOMPRESSED_444p:
				case RAW_FRAME_UNCOMPRESSED_GRAY8:
					MARK("create GLRenderer,type=0x%08x", hwBuffer->frame_type());
					renderer = createGLRenderer(gl_version,
						hwBuffer->frame_type(),
						has_gl_ext_yuv_target && (wrapped->getTexTarget() == GL_TEXTURE_EXTERNAL_OES));
					gl::clear_window(clear_color);
					MEAS_RESET
					break;
				default:
					LOGW("Unsupported frame type!,0x%08x", hwBuffer->frame_type());
					result = USB_ERROR_NOT_SUPPORTED;
					break;
				}
			}
			if (LIKELY(renderer)) {
				result = renderer->draw(wrapped, wrapped->getTexMatrix(), mvp_matrix);
			} else {
				LOGW("Unexpectedly renderer is null!!");
				result = USB_ERROR_NOT_SUPPORTED;
			}
		} else {
			LOGW("Failed to decode mjpeg,err=%d", result);
		}
	}

	if (result) {
		LOGD("処理できなかったときはフォールバックする");
		release_renderer();
		is_hw_buffer_supported = false;
		result = on_draw_mjpeg(frame_mjpeg);
	}

	MEAS_TIME_STOP

	RETURN(result, int);
}
#endif // #if USE_IMAGE_BUFFER
#endif // #if defined(__ANDROID__)


/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_YUYVの時
 * yuyvのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuyv
 * @return recycleの必要なVideoFrameオブジェクト, nullptr可
 */
/*private*/
int VideoGLRenderer::on_draw_yuyv(IVideoFrame &frame_yuyv) {
    ENTER();

    MEAS_TIME_INIT

	int result;

    // 描画用のテクスチャ・レンダーラーの初期化
    if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
        yuvtexture = new gl::GLTexture(
            GL_TEXTURE_2D, GL_TEXTURE0,
            preview_width / 2, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
    }
    if (UNLIKELY(!renderer)) {
    	renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_YUYV);
        // 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
    }

    // 描画処理
    MEAS_TIME_START
    if (LIKELY(frame_yuyv.actual_bytes() == raw_frame_bytes)) {	// 実フレームサイズの比較を追加
        // yuyvフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
        // 元データの2ピクセルがテクスチャの1テクセルに対応するのでテクスチャの横幅を1/2にする
        yuvtexture->assignTexture(frame_yuyv.frame());
        result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);			// ピクセルフォーマットの変換をしながらを描画
    } else {
        MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
            frame_yuyv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
    }
    MEAS_TIME_STOP

	RETURN(result, int);
}

/**
* on_drawの下請け
* preview_frame_type == RAW_FRAME_UYVYの時
* uyvyのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
* @param frame_uyvy
* @return 0: 描画成功, それ以外: エラー
 * @param frame_uyvy
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_uyvy(IVideoFrame &frame_uyvy) {
	ENTER();
	
	MEAS_TIME_INIT
	
	int result;
	
	// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
		  GL_TEXTURE_2D, GL_TEXTURE0,
		  preview_width / 2, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_UYVY);
		// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}
	
	// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_uyvy.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
		// uyvyフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		// 元データの2ピクセルがテクスチャの1テクセルに対応するのでテクスチャの横幅を1/2にする
		yuvtexture->assignTexture(frame_uyvy.frame());
		result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_uyvy.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP
	
	RETURN(result, int);
}

/**
 * on_drawの下請け
* preview_frame_type == RAW_FRAME_YCbCrの時
* YCbCrのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_ycbcr
 * @param result
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_ycbcr(IVideoFrame &frame_ycbcr) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width / 2, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_YCbCr);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_ycbcr.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// yuyvフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
// 元データの2ピクセルがテクスチャの1テクセルに対応するのでテクスチャの横幅を1/2にする
		yuvtexture->assignTexture(frame_ycbcr.frame());
		result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_ycbcr.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_444pの時
 * RAW_FRAME_UNCOMPRESSED_444pのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @param result
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_yuv444p(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height * 3, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_444p);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// yuy444pフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_422pの時
 * RAW_FRAME_UNCOMPRESSED_422pのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @param result
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_yuv422p(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height * 2, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_422p);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YUV422pフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_422sp(YUV422sp)の時
 * RAW_FRAME_UNCOMPRESSED_422spのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @return 0: 描画成功, それ以外: エラー
 */
/*private*/
int VideoGLRenderer::on_draw_yuv422sp(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
// 1枚のテクスチャでフラグメントシェーダーで頑張るより
// yとuvを別々のテクスチャとして渡した方がシェーダーが簡単になる
	// XXX 1280x720ならuse_powered2=trueでも大丈夫だけど全部の解像度で大丈夫がどうか検証できないのでfalseにしておく
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create y texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, false);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!uvtexture)) {
		LOGD("create uv texture");
		uvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE1,
			preview_width / 2, preview_height, USE_PBO,
			GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, false);
		uvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_422sp);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YUV422spフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		uvtexture->assignTexture(&frame_yuv[preview_width * preview_height]);
		result = renderer->draw(yuvtexture, uvtexture, nullptr, mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_I420(YUV420p)の時
 * RAW_FRAME_UNCOMPRESSED_I420のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @param result
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_yuv420p(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, (preview_height * 3) / 2, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_I420);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YUV420pフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_YV12(YUV420p)の時
 * RAW_FRAME_UNCOMPRESSED_I420のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @param result
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_yuv420p_yv12(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, (preview_height * 3) / 2, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_YV12);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YV12フレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		result = renderer->draw(yuvtexture, yuvtexture->getTexMatrix(), mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_NV21(YUV420sp)の時
 * RAW_FRAME_UNCOMPRESSED_NV21のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @return 0: 描画成功, それ以外: エラー
 */
/*private*/
int VideoGLRenderer::on_draw_yuv420sp_nv21(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
// 1枚のテクスチャでフラグメントシェーダーで頑張るより
// yとuvを別々のテクスチャとして渡した方がシェーダーが簡単になる
	// XXX 1280x720ならuse_powered2=trueでも大丈夫だけど全部の解像度で大丈夫がどうか検証できないのでfalseにしておく
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, false);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!uvtexture)) {
		uvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE1,
			preview_width / 2, preview_height / 2, USE_PBO,
			GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, false);
		uvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_NV21);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YUV420pフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		uvtexture->assignTexture(&frame_yuv[preview_width * preview_height]);
		result = renderer->draw(yuvtexture, uvtexture, nullptr, mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_NV12(YUV420sp)の時
 * RAW_FRAME_UNCOMPRESSED_NV12のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
 * @param frame_yuv
 * @return 0: 描画成功, それ以外: エラー
 */
/*private*/
int VideoGLRenderer::on_draw_yuv420sp_nv12(IVideoFrame &frame_yuv) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
// 1枚のテクスチャでフラグメントシェーダーで頑張るより
// yとuvを別々のテクスチャとして渡した方がシェーダーが簡単になる
	// XXX 1280x720ならuse_powered2=trueでも大丈夫だけど全部の解像度で大丈夫がどうか検証できないのでfalseにしておく
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, false);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!uvtexture)) {
		uvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE1,
			preview_width / 2, preview_height / 2, USE_PBO,
			GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, false);
		uvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_NV12);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_yuv.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YUV420pフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_yuv.frame());
		uvtexture->assignTexture(&frame_yuv[preview_width * preview_height]);
		result = renderer->draw(yuvtexture, uvtexture, nullptr, mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_yuv.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/*private*/
int VideoGLRenderer::on_draw_rgbx(IVideoFrame &frame_rgbx) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_RGBX);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_rgbx.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGBXフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_rgbx.frame());
		result = renderer->draw(yuvtexture, nullptr, nullptr, mvp_matrix);
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_rgbx.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/*private*/
int VideoGLRenderer::on_draw_xrgb(IVideoFrame &frame_xrgb) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_XRGB);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_xrgb.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGBXフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_xrgb.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_xrgb.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_RGB565の時
 * @param frame_rgb565
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_rgb565(IVideoFrame &frame_rgb565) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
#if 1
	// RGB565をRGBのテクスチャとしてセットして無変換で貼り付ける
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, false);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_RGB565);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}
#else
	// RGB565をGL_LUMINANCE_ALPHAとしてテクスチャにセットしてフラグメントシェーダーで
	// RGBを取り出して描画する場合...黒い部分の一部に緑色が入ってしまう
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, false);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		MARK("create GLRenderer,gl_version=%d", gl_version);
		if (gl_version >= 300) {
			LOGD("create GLRenderer GLES3");
			renderer = new GLRenderer(texture_gl3_vsh, rgb565_gl3_fsh, USE_VBO);
		} else {
			LOGD("create GLRenderer GLES2");
			renderer = new GLRenderer(texture_gl2_vsh, rgb565_gl2_fsh, USE_VBO);
		}
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}
#endif
// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_rgb565.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGB565フレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_rgb565.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_rgb565.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * 1ピクセル3バイトでアライメント設定1になってしまうので基本的には使わないように
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_RGBの時
 * @param frame_rgb
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_rgb(IVideoFrame &frame_rgb) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_RGB);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_rgb.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGBフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_rgb.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_rgb.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * 1ピクセル3バイトでアライメント設定1になってしまうので基本的には使わないように
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_BGRの時
 * @param frame_bgr
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_bgr(IVideoFrame &frame_bgr) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_BGR);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_bgr.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGBフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_bgr.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_bgr.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_GRAY8の時
 * @param frame_gray8
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_gray8(IVideoFrame &frame_gray8) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO,
			GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_GRAY8);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_gray8.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// YUV420pフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_gray8.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);            // ピクセルフォーマットの変換をしながらを描画
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_gray8.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_BGRXの時
 * @param frame_bgrx
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_bgrx(IVideoFrame &frame_bgrx) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_BGRX);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_bgrx.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGBXフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_bgrx.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_bgrx.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

/**
 * on_drawの下請け
 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_XBGRの時
 * @param frame_xrgb
 * @return
 */
/*private*/
int VideoGLRenderer::on_draw_xbgr(IVideoFrame &frame_xrgb) {
	ENTER();

	MEAS_TIME_INIT

	int result;

// 描画用のテクスチャ・レンダーラーの初期化
	if (UNLIKELY(!yuvtexture)) {
		LOGD("create yuv texture");
		yuvtexture = new gl::GLTexture(
			GL_TEXTURE_2D, GL_TEXTURE0,
			preview_width, preview_height, USE_PBO);
		yuvtexture->setFilter(GL_LINEAR, GL_LINEAR);
	}
	if (UNLIKELY(!renderer)) {
		renderer = createGLRenderer(gl_version, RAW_FRAME_UNCOMPRESSED_XBGR);
// 初回は画面を消去
		gl::clear_window(clear_color);
		MEAS_RESET
	}

// 描画処理
	MEAS_TIME_START
	if (LIKELY(frame_xrgb.actual_bytes() == raw_frame_bytes)) {    // 実フレームサイズの比較を追加
// RGBXフレームデータをテクスチャにセットしてシェーダーを使ってレンダリング
		yuvtexture->assignTexture(frame_xrgb.frame());
		result = renderer->draw(yuvtexture, nullptr, mvp_matrix);
	} else {
		MARK("Unexpected frame bytes: actual_bytes=%" FMT_SIZE_T ",frame_bytes=%" FMT_SIZE_T,
			frame_xrgb.actual_bytes(), raw_frame_bytes);
		result = VIDEO_ERROR_FRAME;
	}
	MEAS_TIME_STOP

	RETURN(result, int);
}

}	// namespace serenegiant::usb

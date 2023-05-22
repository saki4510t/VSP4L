/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_GL_RENDERER_H
#define AANDUSB_VIDEO_GL_RENDERER_H

#include <memory>

// common
#include "gltexture.h"
#include "glrenderer.h"
// core
#include "core/video.h"
#include "core/video_frame_interface.h"
#include "core/video_frame_base.h"
#include "core/video_converter.h"
#include "core/image_buffer.h"

#if defined(__ANDROID__)
#include "core/video_frame_hw_buffer.h"
#endif

namespace serenegiant::core {

/**
 * mjpegからImageBufferへ展開した後ワーク用のBaseVideoFrameへ書き戻して表示するかどうか
 * mjpeg(yuv422p) -> NV22, YV12, I420は正常に表示される
 * false: ImageBufferを使わない, true: ImageBufferを使う
 */
#define USE_IMAGE_BUFFER (false)

class VideoGLRenderer {
private:
	const int gl_version;
	// GL_EXT_YUV_target拡張に対応しているかどうか
	const bool has_gl_ext_yuv_target;
	/**
	 * 画面消去色
	 */
	const uint32_t clear_color;
	VideoConverter converter;
	gl::GLTexture *yuvtexture;
	gl::GLTexture *uvtexture;
	gl::GLRenderer *renderer;
	raw_frame_t preview_frame_type;
	// mjpegのデコード先のフレームタイプ
	raw_frame_t mjpeg_decoded_frame_type;
	// 基本的にはMJPEG_DECODE_TARGETのコピーだけどハードウエアバッファーでANYの時は実際のフレームタイプ
	raw_frame_t mjpeg_decode_target;
	int preview_width, preview_height;
	size_t raw_frame_bytes; // 生フレームデータのサイズ(yuv)
#if defined(__ANDROID__)
	bool is_hw_buffer_supported;
	HWBufferVideoFrame *hwBuffer;
	gl::GLTexture *wrapped;
#endif
	BaseVideoFrame work;
	BaseVideoFrame work1;
	GLfloat mvp_matrix[16]{};
# if USE_IMAGE_BUFFER
	ImageBufferSp imageBuffer;
#endif

	void release_renderer(const bool &release_hw_buffer = true);
	/**
	 * 映像の描画処理
	 * @param frame
	 * @return
	 */
	int on_draw_frame(IVideoFrame &frame);
	/**
	 * on_drawの下請け
	 * MJPEG以外の分岐用
	 * @param frame
	 */
	int on_draw_uncompressed(IVideoFrame &frame);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_MJPEGの時
	 * mjpegのフレームデータをyuv展開してテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_mjpeg
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_mjpeg(IVideoFrame &frame_mjpeg);
#if defined(__ANDROID__)
	/**
	 * on_drawの下請け
	 * ハードウエアバッファに対応している場合
	 * preview_frame_type == RAW_FRAME_MJPEGの時
	 * mjpegのフレームデータをyuv展開してテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_mjpeg
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_mjpeg_hw_buffer(IVideoFrame &frame_mjpeg);
#endif
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_YUYVの時
	 * yuyvのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuyv
	 * @return 0: 描画成功, それ以外: エラー
	 */
    int on_draw_yuyv(IVideoFrame &frame_yuyv);
    /**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UYVYの時
	 * uyvyのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_uyvy
	 * @return 0: 描画成功, それ以外: エラー
     * @param frame_uyvy
     * @return
     */
	int on_draw_uyvy(IVideoFrame &frame_uyvy);
    /**
     * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_YCbCrの時
	 * YCbCrのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
     * @param frame_ycbcr
	 * @return 0: 描画成功, それ以外: エラー
     */
	int on_draw_ycbcr(IVideoFrame &frame_ycbcr);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_444pの時
	 * RAW_FRAME_UNCOMPRESSED_444pのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv444p(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_422pの時
	 * RAW_FRAME_UNCOMPRESSED_422pのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv422p(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_422spの時
	 * RAW_FRAME_UNCOMPRESSED_422spのフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv422sp(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_I420(YUV420p)の時
	 * RAW_FRAME_UNCOMPRESSED_I420のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv420p(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_YV12(YUV420p)の時
	 * RAW_FRAME_UNCOMPRESSED_I420のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv420p_yv12(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_NV21(YUV420sp)の時
	 * RAW_FRAME_UNCOMPRESSED_NV21のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv420sp_nv21(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_NV12(YUV420sp)の時
	 * RAW_FRAME_UNCOMPRESSED_NV12のフレームデータをテクスチャへ転送後GPUでRGBAとして描画する
	 * @param frame_yuv
	 * @return 0: 描画成功, それ以外: エラー
	 */
	int on_draw_yuv420sp_nv12(IVideoFrame &frame_yuv);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_RGBXの時
	 * @param frame_rgbx
	 * @return
	 */
	int on_draw_rgbx(IVideoFrame &frame_rgbx);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_XRGBの時
	 * @param frame_xrgb
	 * @return
	 */
	int on_draw_xrgb(IVideoFrame &frame_xrgb);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_RGB565の時
	 * @param frame_rgb565
	 * @return
	 */
	int on_draw_rgb565(IVideoFrame &frame_rgb565);
	/**
	 * on_drawの下請け
	 * 1ピクセル3バイトでアライメント設定1になってしまうので基本的には使わないように
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_RGBの時
	 * @param frame_rgb
	 * @return
	 */
	int on_draw_rgb(IVideoFrame &frame_rgb);
	/**
	 * on_drawの下請け
	 * 1ピクセル3バイトでアライメント設定1になってしまうので基本的には使わないように
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_BGRの時
	 * @param frame_bgr
	 * @return
	 */
	int on_draw_bgr(IVideoFrame &frame_bgr);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_GRAY8の時
	 * @param frame_gray8
	 * @return
	 */
	int on_draw_gray8(IVideoFrame &frame_gray8);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_BGRXの時
	 * @param frame_bgrx
	 * @return
	 */
	int on_draw_bgrx(IVideoFrame &frame_bgrx);
	/**
	 * on_drawの下請け
	 * preview_frame_type == RAW_FRAME_UNCOMPRESSED_XBGRの時
	 * @param frame_xrgb
	 * @return
	 */
	int on_draw_xbgr(IVideoFrame &frame_xbgr);
protected:
public:
	/**
	 * コンストラクタ
	 * @param gl_version OpenGLのバージョン, OpenGL3.2なら320
	 * @param clear_color 画面消去時の色, ARGB
	 * @param enable_hw_buffer XXX 描画時にAHardwareBufferを使った高速化を有効にする, Androidのみ有効
	 */
	VideoGLRenderer(
		const int &gl_version = 300,
		const uint32_t &clear_color = 0,
		const bool &enable_hw_buffer = false
	);

	/**
	 * デストラクタ
	 */
    virtual ~VideoGLRenderer();
	/**
	 * 関係するリソースを破棄する
	 * @return
	 */
	int release();

	/**
	 * モデルビュー変換行列をセット
	 * @param matrix nullptrなら単位行列になる,それ以外の場合はoffsetから16個以上確保すること
	 * @param offset
	 * @return
	 */
	int set_mvp_matrix(const GLfloat *matrix, const int &offset = 0);

    /**
     * 描画要求
     * @param frame
     * @return
     */
    int draw_frame(IVideoFrame &frame);
};

typedef std::shared_ptr<VideoGLRenderer> VideoGLRendererSp;
typedef std::unique_ptr<VideoGLRenderer> VideoGLRendererUp;

}	// namespace serenegiant::usb

#endif //AANDUSB_VIDEO_GL_RENDERER_H

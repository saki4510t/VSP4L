/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_UVC_PIPELINE_GL_RENDERER_H
#define AANDUSB_UVC_PIPELINE_GL_RENDERER_H

#include <memory>

// common
#include "eglbase.h"
#include "glutils.h"
#include "gltexture.h"
#if __ANDROID__
	// media
	#include "media/surface_texture_java.h"
	#include "media/surface_texture_stub.h"
	#include "media/image_reader.h"
	#include "media/media_codec_decorder.h"
#endif
// core
#include "core/video.h"
#include "core/video_gl_renderer.h"
#include "core/video_frame_queue.h"
#include "core/video_frame_base.h"
// pipeline
#include "pipeline/pipeline_base.h"

namespace serenegiant::pipeline {

typedef void (*on_frame_available)(void);

/**
 * 受け取ったVideoFrameをOpenGLのテクスチャへ描画するためのヘルパークラス
 * H264/VP8はAndroidのAPI>=26(Android8以降)の場合のみ対応する
 */
class GLRendererPipeline : virtual public IPipeline {
private:
	const int gl_version;
	const bool surface_texture_supported;
	const bool image_reader_supported;
	bool keep_last_frame;
	/**
	 * 画面消去色
	 */
	const uint32_t clear_color;
	on_frame_available frame_available;
	core::VideoFrameQueueSp queue;
	/**
	 * 通常描画用
	 */
	core::VideoGLRenderer renderer;
	volatile bool request_change_mvp_matrix;
	core::BaseVideoFrame *last_frame;
	/**
	 * モデルビュー変換行列
	 */
	GLfloat mvp_matrix[16]{};
	int handle_queue_frame(core::BaseVideoFrame *frame);
#if __ANDROID__
	/**
	 * 現在処理中の映像フレームタイプ
	 */
	volatile core::raw_frame_t current_frame_type;
	/**
	 * 現在処理中の映像サイズ(幅)
	 */
	int32_t current_width;
	/**
	 * 現在処理中の映像サイズ(高さ)
	 */
	int32_t current_height;
	/**
	 * h.264/vp8用のデコーダー
	 */
	std::unique_ptr<media::MediaCodecDecoder> decoder;
	/**
	 * H264/VP8で最初のフレームを受信したかどうか
	 */
	bool first_frame;
	/**
	 * H264/VP8はフレームがドロップすると正常にデコードできないので
	 * フレームがドロップしていないかどうかをチェックするためのシーケンス番号
	 */
	uint32_t next_seq;
	/**
	 * H264/VP8をデコードしたテクスチャの描画用
	 */
	gl::GLRendererUp gl_renderer;
	/**
	 * H264/VP8のデコード先テクスチャのラップ用
	 */
	gl::GLTextureUp work;
	/**
	 * H264/VP8デコード先テクスチャ名
	 */
	GLuint m_tex;
	/**
	 * H264/VP8デコード先テクスチャから生成したASurfaceTexture
	 * MediaCodecのハードウエアデコーダーからの映像受け取り用
	 */
	ASurfaceTexture *surface_texture;
	/**
	 * ASurfaceTextureが使えない場合での
	 * MediaCodecのハードウエアデコーダーからの映像受け取り用
	 */
	media::ImageReaderUp image_reader;
	AImage *last_image;
	egl::EglImageWrapperUp wrapper;
	/**
	 * ASurfaceTextureもImageReaderも使えない場合での
	 * MediaCodecのハードウエアデコーダーからの映像受け取り用
	 */
	media::SurfaceTextureJavaUp surface_texture_java;
	core::BaseVideoFrame *on_draw_decoder(core::BaseVideoFrame *frame);
	/**
	 * h.264/vp8表示用のデコーダーを生成する
	 * @param surface_window
	 * @return
	 */
	int create_decoder(const int32_t &width, const int32_t &height);
	/**
	 * h.264/vp8表示用のデコーダーを破棄する
	 */
	void release_decoder();
	int queue_decoder(core::BaseVideoFrame *frame, const core::raw_frame_t &frame_type);
#endif
protected:
	inline void init_pool(const uint32_t &init_num, const size_t &data_bytes = 0) {
		if (queue) queue->init_pool(init_num, data_bytes);
	}
	inline void reset_pool(const size_t &data_bytes = 0) {
		if (queue) queue->reset_pool(data_bytes);
	}
	inline void clear_frames() { if (queue) queue->clear_frames(); };
	inline void clear_pool() { if (queue)  queue->clear_pool(); };
	inline core::BaseVideoFrame *obtain_frame() {
		return queue ? queue->obtain_frame() : nullptr;
	};
	inline int add_frame(core::BaseVideoFrame *frame) {
		return queue ? queue->add_frame(frame) : core::USB_ERROR_ILLEGAL_STATE;
	};
	inline core::BaseVideoFrame *poll_frame() { return queue ? queue->poll_frame() : nullptr; };
	inline core::BaseVideoFrame *wait_frame(const nsecs_t max_wait_ns = 0) {
		return queue ? queue->wait_frame(max_wait_ns) : nullptr;
	};
	inline void recycle_frame(core::BaseVideoFrame *frame) {
		if (LIKELY(queue)) {
			queue->recycle_frame(frame);
		} else {
			SAFE_DELETE(frame);
		}
	};
	inline void signal_queue() { if (queue) queue->signal_queue(); };

	/*Nullable*/
	virtual core::BaseVideoFrame *on_draw_other(/*NonNull*/core::BaseVideoFrame *frame, int &result);
	/**
	 * #start処理の実態
	 * @return
	 */
	int internal_start();
	/**
	 * #stop処理の実態
	 * デストラクタからvirtual関数を呼ぶのは良くないので#stopから分離
	 * @return
	 */
	int internal_stop();
public:
	/**
	 * コンストラクタ
	 * @param gl_version OpenGLのバージョン, OpenGL3.2なら320
	 * @param keep_last_frame 再描画のために最後に処理した映像データを保持するかどうか
	 * @param clear_color 画面消去するときの色, ARGB
	 * @param enable_hw_buffer AHardwareBufferによる描画高速化を有効にするかどうか
	 * @param frame_available 映像データを処理したときのコールバック関数ポインタ
	 * @param gl_version
	 * @param keep_last_frame
	 * @param frame_available
	 * @param queue
	 */
	GLRendererPipeline(
        const int &gl_version, const bool &keep_last_frame,
		const uint32_t &clear_color,
		const bool &enable_hw_buffer,
        on_frame_available frame_available,
		core::VideoFrameQueueSp queue);

	/**
	 * コンストラクタ
	 * @param gl_version OpenGLのバージョン, OpenGL3.2なら320
	 * @param keep_last_frame 再描画のために最後に処理した映像データを保持するかどうか
	 * @param clear_color 画面消去するときの色, ARGB
	 * @param enable_hw_buffer AHardwareBufferによる描画高速化を有効にするかどうか
	 * @param frame_available 映像データを処理したときのコールバック関数ポインタ
	 * @param data_bytes デフォルトのフレームサイズ
	 */
	GLRendererPipeline(
        const int &gl_version = 300, const bool &keep_last_frame = true,
		const uint32_t &clear_color = 0,
		const bool &enable_hw_buffer = false,
        on_frame_available frame_available = nullptr,
        const size_t &data_bytes = DEFAULT_FRAME_SZ);
    /**
     * デストラクタ
     */
	virtual ~GLRendererPipeline();
	/**
	 * 描画処理
	 * イベントループから定期的に呼び出される
	 */
	int on_draw();
	/**
	 * 終了処理
	 * イベントロープ終了時に呼び出される
	 * @return
	 */
	int on_release();

	// IPipelineの純粋仮想関数
	virtual int start() override;
	virtual int stop() override;
	virtual int queue_frame(core::BaseVideoFrame *frame) override;
	/**
	 * モデルビュー変換行列を設定
	 * @param mvp_matrix 要素数16以上
	 * @return
	 */
	int set_mvp_matrix(const GLfloat *mvp_matrix, const int &offset = 0);
};

typedef std::shared_ptr<GLRendererPipeline> GLRendererPipelineSp;
typedef std::unique_ptr<GLRendererPipeline> GLRendererPipelineUp;

}	// end of namespace serenegiant::pipeline

#endif //AANDUSB_UVC_PIPELINE_GL_RENDERER_H

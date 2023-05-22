/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "GLRendererPipeline"

#define COUNT_FRAMES 0

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
#include "core/video_checker.h"
// gl
#include "gl/texture_vsh.h"
#include "gl/rgba_fsh.h"
// media
#if __ANDROID__
#include "media/media_utils.h"
#endif
// pipeline
#include "pipeline/pipeline_gl_renderer.h"

namespace serenegiant::pipeline {

/**
 * コンストラクタ
 * @param gl_version OpenGLのバージョン, OpenGL3.2なら320
 * @param keep_last_frame 再描画のために最後に処理した映像データを保持するかどうか
 * @param clear_color 画面消去するときの色, ARGB
 * @param frame_available 映像データを処理したときのコールバック関数ポインタ
 * @param gl_version
 * @param keep_last_frame
 * @param frame_available
 * @param queue
 */
GLRendererPipeline::GLRendererPipeline(
	const int &gl_version, const bool &keep_last_frame,
	const uint32_t &clear_color,
	const bool &enable_hw_buffer,
	on_frame_available frame_available,
	core::VideoFrameQueueSp queue)
:	IPipeline(),
	gl_version(gl_version),
#if __ANDROID__
	current_frame_type(core::RAW_FRAME_UNKNOWN),
	current_width(0), current_height(0),
	surface_texture_supported(init_surface_texture_ndk()),
	image_reader_supported(init_image_reader_v26()),
	decoder(nullptr),
	first_frame(false), next_seq(0),
	gl_renderer(nullptr),
	work(nullptr),
	m_tex(0),
	surface_texture(nullptr),
	image_reader(nullptr),
	last_image(nullptr),
	surface_texture_java(nullptr),
#else
	surface_texture_supported(false),
	image_reader_supported(false),
#endif
	keep_last_frame(keep_last_frame),
	clear_color(clear_color),
	frame_available(frame_available),
	queue(queue ? std::move(queue) : std::make_shared<core::VideoFrameQueue>()),
	renderer(gl_version, clear_color, enable_hw_buffer),
	request_change_mvp_matrix(false),
	last_frame(nullptr)
{
	ENTER();

	gl::setIdentityMatrix(mvp_matrix, 0);
	set_state(PIPELINE_STATE_INITIALIZED);

	LOGI("surface_texture_supported=%d", surface_texture_supported);
	LOGI("image_reader_supported=%d", image_reader_supported);

	EXIT();
}

/**
 * コンストラクタ
 * @param gl_version OpenGLのバージョン, OpenGL3.2なら320
 * @param keep_last_frame 再描画のために最後に処理した映像データを保持するかどうか
 * @param clear_color 画面消去するときの色, RGBA
 * @param frame_available 映像データを処理したときのコールバック関数ポインタ
 * @param data_bytes デフォルトのフレームサイズ
 */
GLRendererPipeline::GLRendererPipeline(
    const int &gl_version,
	const bool &keep_last_frame,
	const uint32_t &clear_color,
	const bool &enable_hw_buffer,
	on_frame_available frame_available,
    const size_t &data_bytes)
:	GLRendererPipeline(
		gl_version, keep_last_frame, clear_color,
  		enable_hw_buffer,
		frame_available,
		std::make_shared<core::VideoFrameQueue>())
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
GLRendererPipeline::~GLRendererPipeline() {
	ENTER();

	set_state(PIPELINE_STATE_RELEASING);
	internal_stop();
	renderer.release();
#if __ANDROID__
	release_decoder();
#endif

	EXIT();
}

//--------------------------------------------------------------------------------
// IPipelineの純粋仮想関数
/*public*/
int GLRendererPipeline::start() {
	ENTER();
	RETURN(internal_start(), int);
}

// IPipelineの純粋仮想関数
/*public*/
int GLRendererPipeline::stop() {
	ENTER();
	RETURN(internal_stop(), int);
}

// IPipelineの純粋仮想関数
/*public*/
int GLRendererPipeline::queue_frame(core::BaseVideoFrame *_frame) {
//	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
#endif
	if (UNLIKELY(!is_running())) {
		return core::USB_SUCCESS;
	}
	auto frame = dynamic_cast<core::BaseVideoFrame *>(_frame);
	int ret = core::USB_ERROR_OTHER;
	if (LIKELY(frame)) {
		if (LIKELY(is_running())) {
			ret = handle_queue_frame(frame);
#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
			if ((++cnt % 100) == 0) {
    			MARK("cnt=%d", cnt);
			}
#endif
		}
		if (!ret && frame_available) {
			frame_available();
		}
		// 後ろのパイプラインへ繋ぐ
		chain_frame(frame);
	} else {
		LOGW("frame=%p,is_running=%d", frame, is_running());
	}

	return ret;	// RETURN(ret, int);
}

#if __ANDROID__
int GLRendererPipeline::handle_queue_frame(/*NonNull*/core::BaseVideoFrame *frame) {
	ENTER();

	int ret = -1;
	current_frame_type = frame->frame_type();
	current_width = static_cast<int32_t>(frame->width());
	current_height = static_cast<int32_t>(frame->height());
	switch (current_frame_type) {
	case core::RAW_FRAME_H264:
	case core::RAW_FRAME_FRAME_H264:
		keep_last_frame = false;
		ret = queue_decoder(frame, core::RAW_FRAME_H264);
		break;
	case core::RAW_FRAME_VP8:
	case core::RAW_FRAME_FRAME_VP8:
		keep_last_frame = false;
		ret = queue_decoder(frame, core::RAW_FRAME_VP8);
		break;
	case core::RAW_FRAME_H265:
		keep_last_frame = false;
		ret = queue_decoder(frame, core::RAW_FRAME_H265);
		break;
	default:
		decoder.reset();
		break;
	}
	if (!decoder) {
		// フレームプールから空きフレームを取得
		auto copy = obtain_frame();
		if (LIKELY(copy)) {
			// フレームを複製
			*copy = *frame;
			// プレビューフレームキューへ追加
			ret = add_frame(copy);
		} else {
               // 空きフレームを取得できなかった時
               LOGD("buffer pool is empty and exceeds the limit, drop frame");
               ret = core::USB_ERROR_NO_MEM;
           }
	}

	RETURN(ret, int);
}
#else
int GLRendererPipeline::handle_queue_frame(/*NonNull*/core::BaseVideoFrame *frame) {
	ENTER();

	int ret = -1;
	// フレームプールから空きフレームを取得
	auto copy = obtain_frame();
	if (LIKELY(copy)) {
		// フレームを複製
		*copy = *frame;
		// プレビューフレームキューへ追加
		ret = add_frame(copy);
	} else {
		// 空きフレームを取得できなかった時
		LOGD("buffer pool is empty and exceeds the limit, drop frame");
		ret = core::USB_ERROR_NO_MEM;
	}

	RETURN(ret, int);
}
#endif

/**
 * モデルビュー変換行列を設定
 * @param mvp_matrix
 * @return
 */
/*public*/
int GLRendererPipeline::set_mvp_matrix(const GLfloat *mat, const int &offset) {
	ENTER();
	
//	Mutex::Autolock autolock(preview_mutex);

	if (mat) {
		memcpy(mvp_matrix, &mat[offset], sizeof(GLfloat) * 16);
	} else {
		gl::setIdentityMatrix(mvp_matrix, 0);
	}
	request_change_mvp_matrix = true;

	RETURN(core::USB_SUCCESS, int);
}

//--------------------------------------------------------------------------------
#if __ANDROID__
/**
 * 描画処理
 * イベントループから定期的に呼び出される
 */
/*public*/
int GLRendererPipeline::on_draw()
{
	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
#endif
	int result = core::VIDEO_ERROR_NO_DATA;
	if (LIKELY(is_running())) {
		// フレームキューからVideoFrameを取り出す
		core::BaseVideoFrame *frame = poll_frame();
		if (!frame && keep_last_frame) {
			frame = last_frame;
		}
		if (frame || decoder) {
			// XXX フレームデータが無くてもdecoderが有効なときはH264/VP8/H265のはずなので
			//		描画を実行する。でないとダブルバッファの片側にしか描画されずちらついたり
			//		ゴーストのようになる
			if (UNLIKELY(request_change_mvp_matrix)) {
				request_change_mvp_matrix = false;
				renderer.set_mvp_matrix(mvp_matrix, 0);
				LOGV("mvp_mat=%s", mat2String(mvp_matrix, 0).c_str());
			}
			switch (current_frame_type) {
			case core::RAW_FRAME_H264:
			case core::RAW_FRAME_FRAME_H264:
			case core::RAW_FRAME_VP8:
			case core::RAW_FRAME_FRAME_VP8:
			case core::RAW_FRAME_H265:
				frame = on_draw_decoder(frame);
				break;
			default:
				if (frame) {
					result = renderer.draw_frame(*frame);
					if (UNLIKELY(result)) {
						frame = on_draw_other(frame, result);
					}
				}
				break;
			}
			if (keep_last_frame) {
				if (frame != last_frame) {
					core::BaseVideoFrame *t = last_frame;
					last_frame = frame;
					frame = t;
				} else {
					// last_frameと同じなのでリサイクルしないようにnullを代入
					frame = nullptr;
				}
			}
			if (frame) {
				recycle_frame(frame);
			}
#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
			if ((++cnt % 100) == 0) {
				MARK("cnt=%d", cnt);
			}
#endif
		}
	} else {
		gl::clear_window(clear_color);
	}

	RETURN(result, int);
}
#else
/**
 * 描画処理
 * イベントループから定期的に呼び出される
 */
/*public*/
int GLRendererPipeline::on_draw()
{
	ENTER();

#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
    static int cnt = 0;
#endif
	int result = core::VIDEO_ERROR_NO_DATA;
	if (LIKELY(is_running())) {
		// フレームキューからVideoFrameを取り出す
		core::BaseVideoFrame *frame = poll_frame();
		if (!frame && keep_last_frame) {
			frame = last_frame;
		}
		if (LIKELY(frame)) {
			if (UNLIKELY(request_change_mvp_matrix)) {
				request_change_mvp_matrix = false;
				renderer.set_mvp_matrix(mvp_matrix, 0);
				LOGV("mvp_mat=%s", mat2String(mvp_matrix, 0).c_str());
			}
			result = renderer.draw_frame(*frame);
			if (UNLIKELY(result)) {
				frame = on_draw_other(frame, result);
			}
			if (keep_last_frame) {
				if (frame != last_frame) {
					core::BaseVideoFrame *t = last_frame;
					last_frame = frame;
					frame = t;
				} else {
					// last_frameと同じなのでリサイクルしないようにnullを代入
					frame = nullptr;
				}
			}
			if (frame) {
				recycle_frame(frame);
			}
#if COUNT_FRAMES && !defined(LOG_NDEBUG) && !defined(NDEBUG)
			if ((++cnt % 100) == 0) {
				MARK("cnt=%d", cnt);
			}
#endif
		}
	} else {
		gl::clear_window(clear_color);
	}

	RETURN(result, int);
}
#endif

int GLRendererPipeline::on_release() {
	ENTER();

	gl::clear_window(clear_color);

#if __ANDROID__
	wrapper.reset();
	image_reader.reset();
	surface_texture_java.reset();
	if (last_image) {
		AAImage_delete(last_image);
		last_frame = nullptr;
	}
	gl_renderer.reset();
	work.reset();
	if (m_tex) {
		glDeleteTextures(1, &m_tex);
		m_tex = 0;
	}
	if (surface_texture) {
		AASurfaceTexture_release(surface_texture);
		surface_texture = nullptr;
	}
	release_decoder();
#endif
	renderer.release();

	RETURN(0, int);
}

/**
 * preview_frame_typeがRAW_FRAME_MJPEGでもRAW_FRAME_YUYVでも無い時の処理
 * @param frame
 * @return
 */
/*Nullable*/
core::BaseVideoFrame *GLRendererPipeline::on_draw_other(/*NonNull*/core::BaseVideoFrame *frame, int &result) {
	ENTER();
//	clear_window();
	RET(frame);
}


/**
 * #start処理の実態
 * @return
 */
int GLRendererPipeline::internal_start() {
	ENTER();

	int result = EXIT_SUCCESS;
	if (!is_running()) {
		set_running(true);
		set_state(PIPELINE_STATE_STARTING);
		// do something
		set_state(PIPELINE_STATE_RUNNING);
	}

	RETURN(result, int);
}

/**
 * #stop処理の実態
 * デストラクタからvirtual関数を呼ぶのは良くないので#stopから実際の処理を分離
 * @return
 */
int GLRendererPipeline::internal_stop() {
	ENTER();

	bool b = set_running(false);
	if (LIKELY(b)) {
		set_state(PIPELINE_STATE_STOPPING);
		if (last_frame) {
			recycle_frame(last_frame);
			last_frame = nullptr;
		}
		clear_frames();
		clear_pool();
		set_state(PIPELINE_STATE_INITIALIZED);
	}

	RETURN(core::USB_SUCCESS, int);
}

//--------------------------------------------------------------------------------
#if __ANDROID__
core::BaseVideoFrame *GLRendererPipeline::on_draw_decoder(core::BaseVideoFrame *frame) {
	ENTER();

	if (UNLIKELY(!decoder)) {
		create_decoder(current_width, current_height);
	}

	if (LIKELY(surface_texture_supported && surface_texture)) {	// API>=28
		// SurfaceTextureを使うとき
		AASurfaceTexture_updateTexImage(surface_texture);
		if (LIKELY(work && gl_renderer)) {
			work->bind();
			gl_renderer->draw(work.get(), work->getTexMatrix(), mvp_matrix);
		}
	} else if (image_reader) {	// API>=26
		// ImageReaderを使うとき
		auto image = image_reader->get_latest_image();
		if (image) {
			wrapper->unwrap();
			if (last_image) {
				AAImage_delete(last_image);
			}
			last_image = image;
			AHardwareBuffer *buffer = nullptr;
			media_status_t status = AAImage_getHardwareBuffer(image, &buffer);	// API>=26
			if (LIKELY((status == AMEDIA_OK) && buffer)) {
				wrapper->wrap(buffer);
			}
		}
		if (LIKELY(wrapper && gl_renderer)) {
			wrapper->bind();
			gl_renderer->draw(wrapper.get(), wrapper->tex_matrix(), mvp_matrix);
		}
	} else if (surface_texture_java) {
		surface_texture_java->updateTexImage();
		if (LIKELY(work && gl_renderer)) {
			work->bind();
			gl_renderer->draw(work.get(), work->getTexMatrix(), mvp_matrix);
		}
	}

	if (frame) {
		recycle_frame(frame);
		frame = nullptr;
	}

	RET(frame);
}

/**
 * h.264/vp8表示用のデコーダーを生成する
 * ワーカースレッド上で実行される
 * @param surface_window
 * @return
 */
int GLRendererPipeline::create_decoder(const int32_t &width, const int32_t &height) {
	ENTER();

	if (UNLIKELY(!decoder)) {
		ANativeWindow *surface_window = nullptr;
		// OESテクスチャを生成
		MARK("createTexture");
		m_tex = gl::createTexture(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE0, 4);
		if (surface_texture_supported) {	// API>=28
			MARK("ASurfaceTextureが使える時");
			// OESテクスチャからSurfaceTexture/Surface/ANativeWindowsを生成, RGBA8888
			MARK("createSurfaceTexture");
			AutoJNIEnv env;
			surface_texture = media::createSurfaceTexture(env.get(), m_tex, width, height, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
			MARK("surface_texture=%p", surface_texture);
			surface_window = AASurfaceTexture_acquireANativeWindow(surface_texture);
			// 同じテクスチャをGLTextureでラップしてアクセスできるようにする
			MARK("GLTexture::wrap");
			work = std::unique_ptr<gl::GLTexture>(gl::GLTexture::wrap(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE0, m_tex, width, height));
		} else if (image_reader_supported) {	// API>=26
			MARK("ASurfaceTextureが使えないのでImageReaderを使うとき");
			image_reader = std::make_unique<media::ImageReader>(width, height,
				AIMAGE_FORMAT_PRIVATE,
				AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
				4);
			surface_window = image_reader->native_window();
			wrapper = std::make_unique<egl::EglImageWrapper>(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE0, m_tex);
		} else {	// API>=14
			MARK("Java側のSurface/SurfaceTextureへリフレクションでアクセスできる時");
			surface_texture_java = std::make_unique<media::SurfaceTextureJava>(m_tex, width, height, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
			if (surface_texture_java->is_supported()) {
				surface_window = surface_texture_java->native_window();
				// 同じテクスチャをGLTextureでラップしてアクセスできるようにする
				MARK("GLTexture::wrap");
				work = std::unique_ptr<gl::GLTexture>(gl::GLTexture::wrap(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE0, m_tex, width, height));
			} else {
				surface_texture_java.reset();
			}
		}
		
		if (UNLIKELY(surface_window)) {
			// デコード済み映像受け取り用のANativeWindowsを取得できたとき
			MARK("create_decoder");
			first_frame = true;
			next_seq = 0;
			// MediaCodecのハードウエアエアデコーダーを生成
			decoder = std::make_unique<media::MediaCodecDecoder>(surface_window);
			if (gl_version >= 300) {
				LOGD("create GLRenderer GLES3,OES");
				gl_renderer = std::make_unique<gl::GLRenderer>(texture_gl3_vsh, rgba_gl3_ext_fsh, true);
			} else {
				LOGD("create GLRenderer GLES2,OES");
				gl_renderer = std::make_unique<gl::GLRenderer>(texture_gl2_vsh, rgba_gl2_ext_fsh, true);
			}
		} else {
			// デコーダー用にnative windowを生成できなかったのでテクスチャを破棄する
			glDeleteTextures(1, &m_tex);
		}
	}

	RETURN(0, int);
}

/**
 * h.264/vp8表示用のデコーダーを破棄する
 * ワーカースレッド上で実行される
 */
void GLRendererPipeline::release_decoder() {
	ENTER();

	if (decoder) {
		decoder->stop();
		decoder.reset();
	}

	EXIT();
}

int GLRendererPipeline::queue_decoder(core::BaseVideoFrame *frame, const core::raw_frame_t &frame_type) {
	ENTER();

	int result = core::USB_ERROR_OTHER;
	if (LIKELY(decoder && decoder->is_media_ndk_supported())) {
		// native側のMediaCodecで処理できる時
		const uint32_t seq = frame->sequence();
		first_frame |= (seq != next_seq);
		if (!first_frame || core::VideoChecker::is_iframe(*frame)) {
			first_frame = false;
			next_seq = seq + 1;
			if (decoder && !decoder->queue_input(
				frame->frame(), frame->actual_bytes(),
				frame->presentation_time_us(), frame->flags(),
				frame_type, frame->width(), frame->height())) {
				result = core::USB_SUCCESS;
			}
		}
	}

	RETURN(result, int);
}
#endif

}	// end of namespace serenegiant::pipeline

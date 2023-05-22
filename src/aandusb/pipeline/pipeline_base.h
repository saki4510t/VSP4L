/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef PUPILMOBILE_IPIPELINE_H
#define PUPILMOBILE_IPIPELINE_H

#include <stdlib.h>

// core
#include "core/core.h"
#include "core/video_frame_base.h"

#pragma interface

namespace serenegiant::pipeline {

/**
 * デフォルトのフレームサイズ
 */
#define DEFAULT_FRAME_SZ 1024

/**
 * パイプラインの種類
 */
typedef enum pipeline_type {
	PIPELINE_TYPE_SIMPLE_BUFFERED = 0,
	PIPELINE_TYPE_UVC_CONTROL = 100,
	PIPELINE_TYPE_PTS_CALC = 200,
	PIPELINE_TYPE_CALLBACK = 300,
	PIPELINE_TYPE_CALLBACK_NATIVE = 310,
	PIPELINE_TYPE_DISTRIBUTE = 400,
	PIPELINE_TYPE_PREVIEW_GL_RENDERER = 510,
	PIPELINE_TYPE_PREVIEW_GL_RENDERER_UPC = 520,	// deprecated
	PIPELINE_TYPE_GL_PREVIEW = 530,
	PIPELINE_TYPE_CONVERT = 600,
} pipeline_type_t;

#define PIPELINE_TYPE_PREVIEW_GL_RENDERER_UVC (PIPELINE_TYPE_PREVIEW_GL_RENDERER)

/**
 * パイプラインのステート
 */
typedef enum _pipeline_state {
	PIPELINE_STATE_UNINITIALIZED = 0,
	PIPELINE_STATE_RELEASING = 10,
	PIPELINE_STATE_INITIALIZED = 20,
	PIPELINE_STATE_STARTING = 30,
	PIPELINE_STATE_RUNNING = 40,
	PIPELINE_STATE_STOPPING = 50,
} pipeline_state_t;

class IPipeline;

/**
 * パイプラインの親(前)を探すためのヘルパークラス
 * パイプラインの親がIPipelineとは限らない(例えばPipelineSource)なので
 * IPipelineとは別クラスにしている
 */
class IPipelineParent {
friend IPipeline;
private:
	/**
	 * 親パイプライン
	 */
	IPipelineParent *_parent;
protected:
	mutable Mutex parent_mutex;
public:
	/**
	 * コンストラクタ
	 */
	IPipelineParent();
	/**
	 * デストラクタ
	 */
	virtual ~IPipelineParent();
	/**
	 * 親パイプラインをセット
	 * @param parent
	 * @return
	 */
	virtual IPipelineParent *setParent(IPipelineParent *parent);
	/**
	 * 親パイプラインを取得
	 * @return @Nullable
	 */
	virtual IPipelineParent *getParent();
};

class DistributePipeline;
class PipelineSource;

/**
 * パイプライン処理の基底クラス
 */
class IPipeline : virtual public IPipelineParent {
friend DistributePipeline;
friend PipelineSource;
private:
	/**
	 * パイプライン処理実行中フラグ
	 */
	volatile bool mIsRunning;
	/**
	 * パイプラインのステート
	 */
	volatile pipeline_state_t state;
	// force inhibiting copy/assignment
	/**
	 * コピーコンストラクタ
	 * コピー・代入できないようにするためprivate
	 * @param src
	 */
	IPipeline(const IPipeline &src) = delete;
	/**
	 * 代入演算子
	 * コピー・代入できないようにするためprivate
	 * @param src
	 */
	void operator =(const IPipeline &src) = delete;
protected:
	/**
	 * パイプラインの排他制御用ミューテックス
	 */
	mutable Mutex pipeline_mutex;
	/**
	 * 次のパイプライン, @Nullable
	 */
	IPipeline *next_pipeline;

	/**
	 * コンストラクタ
	 */
	IPipeline();
	/**
	 * デストラクタ
	 */
	virtual ~IPipeline();
	/**
	 * ステートをセット
	 * @param new_state
	 */
	void set_state(const pipeline_state_t &new_state);
	/**
	 * 実行中かどうかをセット
	 * @param is_running
	 * @return
	 */
	inline bool set_running(const bool &is_running) {
		const bool prev = mIsRunning;
		mIsRunning  = is_running;
		return (prev);
	};
	/**
	 * 次のパイプラインへフレームを送る
	 * 次のパイプラインの::queue_frameを呼び出す
	 * @param frame
	 * @return
	 */
	virtual int chain_frame(core::BaseVideoFrame *frame);
	/**
	 * パイプライン処理実行中にリセットが必要になったときの処理
	 * 前のパイプラインから呼ばれる
	 */
	virtual void on_reset();
	/**
	 * カメラ側で静止画キャプチャを実行する時(メソッド２または３)のコールバック関数
	 * 前のパイプラインから呼ばれる
	 * @param frame
	 */
	virtual void on_capture(core::BaseVideoFrame *frame);
public:
	/**
	 * 現在の状態を取得
	 * @return
	 */
	pipeline_state_t get_state() const;
	/**
	 * パイプライン処理を実行中かどうか
	 * @return
	 */
	inline const bool is_running() const { return mIsRunning; };
	/**
	 * 次のパイプラインをセット
	 * @param pipeline @Nullable
	 * @return
	 */
	virtual int set_pipeline(IPipeline *pipeline) final;
	/**
	 * パイプライン処理を開始
	 * @return
	 */
	virtual int start() = 0;
	/**
	 * パイプライン処理を停止
	 * @return
	 */
	virtual int stop() = 0;
	/**
	 * 新しいフレームを受け取る処理
	 * @param frame
	 * @return
	 */
	virtual int queue_frame(core::BaseVideoFrame *frame) = 0;
};

}	// end of namespace serenegiant::pipeline

#endif //PUPILMOBILE_IPIPELINE_H

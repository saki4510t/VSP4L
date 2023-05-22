/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1	// set 1 if you don't need debug message
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// ignore LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG		// depends on definition in Android.mk and Application.mk
#endif

#include <cstdlib>

#include "utilbase.h"
// pipeline
#include "pipeline/pipeline_base.h"

namespace serenegiant::pipeline {

/**
 * コンストラクタ
 */
/*public*/
IPipelineParent::IPipelineParent()
:	_parent(nullptr)
{
	ENTER();

	EXIT();
}

/**
 * デストラクタ
 */
/*public*/
IPipelineParent::~IPipelineParent() {
	ENTER();

	Mutex::Autolock lock(parent_mutex);
	_parent = nullptr;

	EXIT();
}

/**
 * 親パイプラインをセット
 * @param parent
 * @return
 */
/*public*/
IPipelineParent *IPipelineParent::setParent(IPipelineParent *parent) {
	ENTER();

	Mutex::Autolock lock(parent_mutex);
	_parent = parent;

	RET(this);
}

/**
 * 親パイプラインを取得
 * @return @Nullable
 */
/*public*/
IPipelineParent *IPipelineParent::getParent() {
	ENTER();

	Mutex::Autolock lock(parent_mutex);

	RET(_parent);
}

//********************************************************************************
//********************************************************************************

/**
 * コンストラクタ
 * @param default_frame_size, デフォルトはDEFAULT_FRAME_SZ
 */
/*protected*/
IPipeline::IPipeline()
:	IPipelineParent(),
	mIsRunning(false),
	state(PIPELINE_STATE_UNINITIALIZED),
	next_pipeline(nullptr)
{
	ENTER();

	EXIT();
}

/**
 * デストラクタ
 */
/*virtual, protected*/
IPipeline::~IPipeline() {
	ENTER();
	
	set_pipeline(nullptr);
	set_state(PIPELINE_STATE_UNINITIALIZED);

	EXIT();
}

/**
 * 現在の状態を取得
 * @return
 */
/*public*/
pipeline_state_t IPipeline::get_state() const { return (state); };

/**
 * ステートをセット
 * @param new_state
 */
/*protected*/
void IPipeline::set_state(const pipeline_state_t &new_state) { state = new_state; }

/**
 * 次のパイプラインをセット
 * @param pipeline @Nullable
 * @return
 */
/*public*/
int IPipeline::set_pipeline(IPipeline *pipeline) {
	ENTER();

	pipeline_mutex.lock();
	{
		// Java側で保持/ライフサイクルを管理しているので、元の値は気にせず上書きする
//		if (next_pipeline) {
//			next_pipeline->setParent(nullptr);
//		}
		next_pipeline = pipeline;
	}
	pipeline_mutex.unlock();

	if (pipeline) {
		pipeline->setParent(this);
	}

	RETURN(0, int);
}

/**
 * 次のパイプラインへフレームを送る
 * 次のパイプラインの::queue_frameを呼び出す
 * @param frame
 * @return
 */
/*virtual, protected*/
int IPipeline::chain_frame(core::BaseVideoFrame *frame) {
//	ENTER();

	int result = core::USB_SUCCESS;
	pipeline_mutex.lock();
	{
		if (is_running() && next_pipeline) {
			try {
				result = next_pipeline->queue_frame(frame);
			} catch (...) {
				LOGW("exception caught!!");
			}
		} else if (!is_running()) {
			result = core::USB_ERROR_ILLEGAL_STATE;
		}
	}
	pipeline_mutex.unlock();

	return result; // RETURN(result, int);
}

/**
 * パイプライン処理実行中にリセットが必要になったときの処理
 * 前のパイプラインから呼ばれる
 */
/*virtual, protected*/
void IPipeline::on_reset() {
	ENTER();

	pipeline_mutex.lock();
	{
		if (is_running() && next_pipeline) {
			next_pipeline->on_reset();
		}
	}
	pipeline_mutex.unlock();

	EXIT();
}

/**
 * 静止画撮影実行時の処理
 * 前のパイプラインから呼ばれる
 * XXX 削除するかも
 */
/*virtual, protected*/
void IPipeline::on_capture(core::BaseVideoFrame *frame) {
	ENTER();

	pipeline_mutex.lock();
	{
		if (is_running() && next_pipeline) {
			next_pipeline->on_capture(frame);
		}
	}
	pipeline_mutex.unlock();

	EXIT();
}

}	// end of namespace serenegiant::pipeline

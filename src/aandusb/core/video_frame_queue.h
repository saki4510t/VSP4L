/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_FRAME_QUEUE_H
#define AANDUSB_VIDEO_FRAME_QUEUE_H

#include <memory>

// core
#include "core/frame_queue.h"
#include "core/video_frame_base.h"

namespace serenegiant::core {

typedef BaseVideoFrame *(*frame_factory_t)(const size_t &data_bytes);
typedef BaseVideoFrameSp (*frame_sp_factory_t)(const size_t &data_bytes);

class VideoFrameQueue  : public FrameQueue<BaseVideoFrame *> {
private:
	frame_factory_t _frame_factory;
public:
	/**
	 * コンストラクタ
	 * @param max_frame_num
	 * @param init_frame_num
	 * @param _default_frame_sz
	 * @param create_if_empty
	 * @param block_if_empty
	 * @param dct_mode
	 */
	VideoFrameQueue(const uint32_t &max_frame_num = DEFAULT_MAX_FRAME_NUM,
		const uint32_t &init_frame_num = DEFAULT_INIT_FRAME_POOL_SZ,
		const size_t &_default_frame_sz = DEFAULT_FRAME_SZ,
		const bool &create_if_empty = false, const bool &block_if_empty = false,
		frame_factory_t frame_factory = nullptr)
	: FrameQueue<BaseVideoFrame *>(max_frame_num, init_frame_num,
		_default_frame_sz, create_if_empty, block_if_empty),
		_frame_factory(frame_factory)
	{
		ENTER();
		EXIT();
	}

	/**
	 * デストラクタ
	 */
	virtual ~VideoFrameQueue() {
		ENTER();
		EXIT();
	}

	void set_factory(frame_factory_t factory) {
		ENTER();
		Mutex::Autolock lock(pool_mutex);
		_frame_factory = factory;

		EXIT();
	}

	/**
	 * FramePool::create_frameの実装
	 * 新規フレーム生成が必要なときの処理
	 * pool_mutexがロックされた状態で呼ばれる
	 * @param data_bytes
	 * @return
	 */
	virtual BaseVideoFrame *create_frame(const size_t &data_bytes) override {
		ENTER();
		RET(_frame_factory ? _frame_factory(data_bytes) : new BaseVideoFrame(data_bytes));
	};

	/**
	 * フレームの破棄が必要なときの処理
	 * @param frame
	 */
	virtual void delete_frame(BaseVideoFrame *frame) override {
		ENTER();
		SAFE_DELETE(frame);
		EXIT();
	}
};

class VideoFrameSpQueue : public FrameQueue<BaseVideoFrameSp> {
private:
	frame_sp_factory_t _frame_factory;
public:
	/**
	 * コンストラクタ
	 * @param max_frame_num
	 * @param init_frame_num
	 * @param _default_frame_sz
	 * @param create_if_empty
	 * @param block_if_empty
	 * @param dct_mode
	 */
	VideoFrameSpQueue(const uint32_t &max_frame_num = DEFAULT_MAX_FRAME_NUM,
		const uint32_t &init_frame_num = DEFAULT_INIT_FRAME_POOL_SZ,
		const size_t &_default_frame_sz = DEFAULT_FRAME_SZ,
		const bool &create_if_empty = false, const bool &block_if_empty = false,
		frame_sp_factory_t frame_factory = nullptr)
	: FrameQueue<BaseVideoFrameSp>(max_frame_num, init_frame_num,
		_default_frame_sz, create_if_empty, block_if_empty),
		_frame_factory(frame_factory)
	{
		ENTER();
		EXIT();
	}

	/**
	 * デストラクタ
	 */
	virtual ~VideoFrameSpQueue() {
		ENTER();
		EXIT();
	}

	void set_factory(frame_sp_factory_t factory) {
		ENTER();
		Mutex::Autolock lock(pool_mutex);
		_frame_factory = factory;

		EXIT();
	}

	/**
	 * FramePool::create_frameの実装
	 * 新規フレーム生成が必要なときの処理
	 * pool_mutexがロックされた状態で呼ばれる
	 * @param data_bytes
	 * @return
	 */
	virtual BaseVideoFrameSp create_frame(const size_t &data_bytes) override {
		ENTER();
		RET(_frame_factory ? _frame_factory(data_bytes) : std::make_shared<BaseVideoFrame>(data_bytes));
	};


	/**
	 * フレームの破棄が必要なときの処理
	 * @param frame
	 */
	virtual void delete_frame(BaseVideoFrameSp frame) override {
		// 所有権を破棄する
		frame.reset();
	}
};

typedef std::shared_ptr<VideoFrameQueue> VideoFrameQueueSp;
typedef std::unique_ptr<VideoFrameQueue> VideoFrameQueueUp;
typedef std::shared_ptr<VideoFrameSpQueue> VideoFrameSpQueueSp;
typedef std::unique_ptr<VideoFrameSpQueue> VideoFrameSpQueueUp;

}	// namespace serenegiant::usb

#endif //AANDUSB_VIDEO_FRAME_QUEUE_H

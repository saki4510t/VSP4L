/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_FRAME_QUEUE_H
#define AANDUSB_FRAME_QUEUE_H

#include <list>

#include "utilbase.h"

// core
#include "core/core.h"
#include "core/frame_pool.h"

namespace serenegiant::core {

#define MAX_FRAME_PREVIEW 4
#define FRAME_POOL_SZ MAX_FRAME_PREVIEW + 2

/**
 * FramePoolを継承したフレームキューテンプレート
 * @tparam T
 */
template <typename T>
class FrameQueue : public FramePool<T> {
private:
	mutable Mutex queue_mutex;
	Condition queue_sync;
	std::list<T> frame_queue;	// FIFOにしないといけないのでstd::listを使う, std::queueでもいいかも
	/**
	 * コピーコンストラクタ
	 * (コピー禁止)
	 * @param src
	 */
	FrameQueue(const FrameQueue &src) = delete;
	/**
	 * 代入演算子
	 * (代入禁止)
	 * @param src
	 * @return
	 */
	FrameQueue &operator=(const FrameQueue &src) = delete;
	/**
	 * ムーブコンストラクタ
	 * (ムーブ禁止)
	 * @param src
	 */
	FrameQueue(FrameQueue &&src) = delete;
	/**
	 * ムーブ代入演算子
	 * (代入禁止)
	 * @param src
	 * @return
	 */
	FrameQueue &operator=(const FrameQueue &&src) = delete;
protected:
	// 継承コンストラクタ
	using FramePool<T>::FramePool;
	
	/**
	 * デストラクタ
	 */
	virtual ~FrameQueue() {
		ENTER();

		clear_frames();

		EXIT();
	}

public:
	/**
	 * フレームキューをクリア
	 */
	void clear_frames() {
		ENTER();
	
		std::list<T>temp;
		queue_mutex.lock();
		{
			temp = frame_queue;		// コピー
			frame_queue.clear();	// 元のはクリアする
		}
		queue_mutex.unlock();
		// コピーしたのをリサイクルする
		for (auto item : temp) {
			FramePool<T>::recycle_frame(item);
		}
	
		EXIT();
	}

	/**
	 * キューのサイズが最大フレーム吸うより大きいときには古いフレームを破棄する
	 * @param margin
	 * @return
	 */
	int check_queue_size(const int &margin = 1) {
		ENTER();

		int result = USB_SUCCESS;
		queue_mutex.lock();
		{
			if (frame_queue.size() >= FramePool<T>::get_max_frame_num()) {
				// キューのサイズが最大フレーム数より大きい時
				// 古いフレームを破棄する
				const int n = frame_queue.size() - FramePool<T>::get_max_frame_num() + (margin > 0 ? margin : 1);
				int cnt = 0;
				for (auto iter = frame_queue.begin();
					(iter != frame_queue.end()) && (cnt < n); iter++, cnt++) {

					FramePool<T>::recycle_frame(*iter);
					iter = frame_queue.erase(iter);
				}
				LOGW("dropped frame data(%d)", cnt);
			}
		}
		queue_mutex.unlock();

		RETURN(result, int);
	}

	/**
	 * フレームキューに追加
	 * @param frame
	 * @return
	 */
	int add_frame(T frame) {
		ENTER();
	
		int result = USB_SUCCESS;
		queue_mutex.lock();
		{
			if (frame_queue.size() >= FramePool<T>::get_max_frame_num()) {
				// キューのサイズが最大フレーム数より大きい時
				// 古いフレームを破棄する
				const int n = frame_queue.size() - FramePool<T>::get_max_frame_num() + 1;
				int cnt = 0;
				for (auto iter = frame_queue.begin();
					(iter != frame_queue.end()) && (cnt < n); iter++, cnt++) {

					FramePool<T>::recycle_frame(*iter);
					iter = frame_queue.erase(iter);
				}
				LOGW("%d frame(s) dropped", cnt);
			}
			if ((frame_queue.size() < FramePool<T>::get_max_frame_num())) {
				frame_queue.push_back(frame);
				frame = nullptr;	// 正常にキューに追加できた
			}
			queue_sync.signal();
		}
		queue_mutex.unlock();

		if (frame) {
			// キューに追加できなかった時
			FramePool<T>::recycle_frame(frame);
			result = USB_ERROR_NO_SPACE;
		}
	
		RETURN(result, int);
	}

	/**
	 * フレームキューからフレームを取り出す。空なら待機する
	 * 待機解除または最大待ち時間を経過してもキューが空ならnullptrを返す
	 * @param max_wait_ns最大待ち時間, 0以下なら無限待ち
	 * @return
	 */
	/*@Nullable*/
	T wait_frame(const nsecs_t max_wait_ns = 0) {

		ENTER();

		T frame = nullptr;
		queue_mutex.lock();
		{
			if (UNLIKELY(frame_queue.empty())) {
				// キューにフレームがなければ待機する
				if (max_wait_ns > 0) {
					queue_sync.waitRelative(queue_mutex, max_wait_ns);
				} else {
					queue_sync.wait(queue_mutex);
				}
			}
			if (LIKELY(!frame_queue.empty())) {
				frame = frame_queue.front();	// 先頭・・・一番古いフレームを取り出す
				frame_queue.pop_front();
			}
		}
		queue_mutex.unlock();

		RET(frame);
	}

	/**
	 * フレームキューにフレームがあれば取り出す、なければnullptr
	 * @return
	 */
	/*@Nullable*/
	T poll_frame() {
		ENTER();
		
		T frame = nullptr;
		queue_mutex.lock();
		{
			if (LIKELY(!frame_queue.empty())) {
				frame = frame_queue.front();	// 先頭・・・一番古いフレームを取り出す
				frame_queue.pop_front();
			}
		}
		queue_mutex.unlock();

		RET(frame);
	}

	/**
	 * フレームキュー中のフレーム数を取得する
	 * @return
	 */
	size_t get_frame_count() {
		ENTER();
	
		size_t result;
		queue_mutex.lock();
		{
			result = frame_queue.size();
		}
		queue_mutex.unlock();
	
		RETURN(result, size_t);
	}

	/**
	 * フレームキューの待機(wait_frame)を解除する
	 */
	void signal_queue() {
		ENTER();

		queue_mutex.lock();
		{
			queue_sync.broadcast();
		}
		queue_mutex.unlock();

		EXIT();
	}
};


}	// end of namespace serenegiant::usb

#endif //AANDUSB_FRAME_QUEUE_H

/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef USBWEBCAMERAPROJ_FRAME_POOL_H
#define USBWEBCAMERAPROJ_FRAME_POOL_H

#include <vector>
#include <algorithm>

#include "utilbase.h"

// core
#include "core/core.h"

namespace serenegiant::core {

#define DEFAULT_INIT_FRAME_POOL_SZ 2
#define DEFAULT_MAX_FRAME_NUM 8
#define DEFAULT_FRAME_SZ 1024

/**
 * フレームプールテンプレート
 * メモリアロケーションの回数を減らしてスピードアップ・・・実測で5〜30%ぐらい速くなるみたい
 * @tparam T
 */
template <typename T>
class FramePool {
private:
	const uint32_t MAX_FRAME_NUM;			// 最大フレーム数
	const uint32_t INIT_FRAME_NUM;			// 初期フレーム数
	const bool CREATE_IF_EMPTY;				// プールが空の時に自動生成するかどうか
	const bool BLOCK_IF_EMPTY;				// プールが空の時にブロックしてrecycleされるまで待機するかどうか
	size_t default_frame_sz;				// フレーム生成時の初期サイズ
	volatile uint32_t total_frame_num;		// 生成されたフレームの個数
	volatile bool initialized;				// 初期化済みかどうか
	volatile bool cleared;					// フレームバッファがクリアされたかどうか
	Condition pool_sync;					// 同期オブジェクト
	std::vector<T> frame_pool;			// フレームプール
	/**
	 * コピーコンストラクタ
	 * (コピー禁止)
	 * @param src
	 */
	FramePool(const FramePool &src) = delete;
	/**
	 * 代入演算子
	 * (代入禁止)
	 * @param src
	 * @return
	 */
	FramePool &operator=(const FramePool &src) = delete;
	/**
	 * ムーブコンストラクタ
	 * (ムーブ禁止)
	 * @param src
	 */
	FramePool(FramePool &&src) = delete;
	/**
	 * ムーブ代入演算子
	 * (代入禁止)
	 * @param src
	 * @return
	 */
	FramePool &operator=(const FramePool &&src) = delete;
protected:
	mutable Mutex pool_mutex;				// ミューテックス
	/**
	 * 新規フレーム生成が必要なときの処理
	 * pool_mutexがロックされた状態で呼ばれる
	 * @param data_bytes
	 * @return
	 */
	virtual T create_frame(const size_t &data_bytes) = 0;
	/**
	 * フレームの破棄が必要なときの処理
	 * @param frame
	 */
	virtual void delete_frame(T frame) = 0;
public:
	/**
	 * コンストラクタ
	 * @param max_frame_num
	 * @param init_frame_num
	 * @param _default_frame_sz
	 * @param create_if_empty
	 * @param block_if_empty
	 */
	FramePool(const uint32_t &max_frame_num,
		const uint32_t &init_frame_num,
		const size_t &_default_frame_sz,
		const bool &create_if_empty, const bool &block_if_empty)
	:	MAX_FRAME_NUM(max_frame_num > 0 ? max_frame_num : DEFAULT_MAX_FRAME_NUM),
		INIT_FRAME_NUM(init_frame_num < MAX_FRAME_NUM ? init_frame_num : MAX_FRAME_NUM),
		CREATE_IF_EMPTY(create_if_empty),
		BLOCK_IF_EMPTY(block_if_empty),
		default_frame_sz(_default_frame_sz > 0 ? _default_frame_sz : DEFAULT_FRAME_SZ),
		total_frame_num(0),
		initialized(false), cleared(true) {

		ENTER();
		EXIT();
	}

	/**
	 * デストラクタ
	 */
	virtual ~FramePool() {
		ENTER();

//		clear_pool();

		EXIT();
	}

	const uint32_t get_max_frame_num() const { return MAX_FRAME_NUM; }

	/**
	 * デフォルトのフレームサイズを取得
	 * @return
	 */
	const size_t get_default_frame_sz() const { return default_frame_sz; };

	/**
	 * フレームプールを初期化
	 * すでに初期化済みの場合はクリアした後再度初期化する
	 * @param init_num 0以下なら何もしない
	 * @param data_bytes 0なら前回のデータサイズを使う
	 */
	virtual void init_pool(const uint32_t &init_num, const size_t &data_bytes = 0) {
		ENTER();

		clear_pool();
		if (LIKELY(init_num > 0)) {
			pool_mutex.lock();
			{
				if (data_bytes) {
					default_frame_sz = data_bytes;
				}
				for (uint32_t i = 0; i < init_num; i++) {
					frame_pool.push_back(create_frame(default_frame_sz));
					total_frame_num++;
				}
				initialized = true;
				cleared = false;
				pool_sync.broadcast();
			}
			pool_mutex.unlock();
		}

		EXIT();
	}

	/**
	 * 最大フレーム数でフレームプールを初期化
	 * すでに初期化済みの場合はクリアした後再度初期化する
	 */
	virtual void fill_pool() {
		init_pool(MAX_FRAME_NUM, 0);
	}

	/**
	 * フレームプールを指定したデータサイズで再初期化する
	 * @param data_bytes
	 */
	virtual void reset_pool(const size_t &data_bytes = 0) {
		ENTER();

		init_pool(INIT_FRAME_NUM, data_bytes);

		EXIT();
	}

	/**
	 * プール内のフレームをすべて破棄する
	 */
	void clear_pool() {
		ENTER();

		Mutex::Autolock autolock(pool_mutex);
		cleared = true;
		pool_sync.broadcast();
		for (auto item : frame_pool) {
			if (LIKELY(item)) {
				delete_frame(item);
			}
		}
		frame_pool.clear();
		total_frame_num = 0;

		EXIT();
	}

	/**
	 * プールからフレームの取得を試みる
	 * プールが空でBLOCK_IF_EMPTY=trueなら取得できるまで待機する
	 * プールが空でBLOCK_IF_EMPTY=falseでCREATE_IF_EMPTYなら新規作成する
	 * プールが空でBLOCK_IF_EMPTY=falseでCREATE_IF_EMPTY=falseならnullptrを返す
	 * @return
	 */
	virtual T obtain_frame() {
		ENTER();

		if (UNLIKELY(!initialized)) {
			reset_pool(default_frame_sz);
		}

		T frame = nullptr;
		pool_mutex.lock();
		{
			if (UNLIKELY(frame_pool.empty() && (total_frame_num < MAX_FRAME_NUM))) {
				// 頻繁にプールサイズを拡張しないように前回の2倍になるように試みる
				uint32_t n = total_frame_num ? total_frame_num * 2 : 2;
				if (n > MAX_FRAME_NUM) {
					// 上限を超えないように制限する
					n = MAX_FRAME_NUM;
				}
				if (LIKELY(n > total_frame_num)) {
					// 新規追加
					n -= total_frame_num;
					for (uint32_t i = 0; i < n; i++) {
						frame_pool.push_back(create_frame(default_frame_sz));
						total_frame_num++;
					}
					LOGW("allocate new frame(s):total=%d", total_frame_num);
				} else {
					LOGW("number of allocated frame exceeds limit");
				}
			}
			if (UNLIKELY(frame_pool.empty())) {
				if (BLOCK_IF_EMPTY) {
					for (; !cleared && frame_pool.empty() ; ) {
						pool_sync.wait(pool_mutex);
					}
				} else if (CREATE_IF_EMPTY) {
					frame_pool.push_back(create_frame(default_frame_sz));
					total_frame_num++;
					LOGW("allocate new frame:total=%d", total_frame_num);
				}
			}
			if (!frame_pool.empty()) {
				frame = frame_pool.back();
				frame_pool.pop_back();
			}
		}
		pool_mutex.unlock();

		RET(frame);
	}

	/**
	 * 指定したフレームをプールに返却する
	 * @param frame
	 */
	virtual void recycle_frame(T frame) {
		ENTER();

		if (LIKELY(frame)) {
			pool_mutex.lock();
			{
				if (LIKELY(frame_pool.size() < MAX_FRAME_NUM)) {
					frame_pool.push_back(frame);
					frame = nullptr;
				}
				if (UNLIKELY(frame)) { // frameプールに戻せなかった時
					delete_frame(frame);
					total_frame_num--;
				}
				pool_sync.signal();
			}
			pool_mutex.unlock();
		}

		EXIT();
	}

	/**
	 * 指定したフレームをプールから削除して合計フレーム数を減らす
	 * @param frame
	 */
	virtual void remove(T frame) {
		ENTER();
	
		if (frame) {
			pool_mutex.lock();
			{
				auto found = std::find(frame_pool.begin(), frame_pool.end(), frame);
				if (found != frame_pool.end()) {
					frame_pool.erase(found);
				}
				total_frame_num--;
			}
			pool_mutex.unlock();
			
			delete_frame(frame);
		}

		EXIT();
	}
};

}	// end of namespace serenegiant::usb

#endif //USBWEBCAMERAPROJ_FRAME_POOL_H

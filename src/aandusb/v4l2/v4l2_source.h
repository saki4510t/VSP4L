/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_V4L2_SOURCE_H
#define AANDUSB_V4L2_SOURCE_H

#include <functional>
#include <stddef.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// common
#include "mutex.h"
#include "condition.h"
// v4l2
#include "v4l2/v4l2.h"
#include "v4l2/v4l2_ctrl.h"

namespace uvc = serenegiant::usb::uvc;

namespace serenegiant::v4l2 {

/**
 * 映像データ最大待ち時間
 */
#define MAX_WAIT_FRAME_US (100000L)

/**
 * 映像データ受け取り用バッファーの個数
 */
#define DEFAULT_BUFFER_NUMS (4)

/**
 * @brief V4L2から映像を取得するためのヘルパークラス
 *
 */
class V4l2SourceBase: public virtual V4L2Ctrl {
private:
	// v4l2機器名
	const std::string device_name;
	// udmabufデバイスファイル名
	const std::string udmabuf_name;
	/**
	 * @brief 専用ワーカースレッドからhandle_frameを呼び出すかどうか
	 *        1: 専用ワーカースレッドを使う,
	 *        0: 専用ワーカースレッドを使わない(自前でhandle_frameを呼び出さないといけない)
	 */
	const bool async;

	/**
	 * パイプライン処理実行中フラグ
	 */
	volatile bool m_running;
	/**
	 * v4l2機器のファイルディスクリプタ
	 */
	volatile int m_fd;
	/**
	 * v4l2の状態
	 */
	state_t m_state;
	/**
	 * udmabufのファイルディスクリプタ
	 */
	int m_udmabuf_fd;
	/**
	 * リサイズ要求フラグ
	 */
	volatile bool request_resize;
	/**
	 * リサイズ要求値
	 */
	uint32_t request_width, request_height;
	/**
	 * ピクセルフォーマット要求値
	 */
	uint32_t request_pixel_format;
	/**
	 * フレームレート要求値
	 */
	float request_min_fps, request_max_fps;
	/**
	 * 映像サイズ
	 */
	uint32_t stream_width, stream_height;
	/**
	 * ピクセルフォーマット
	 */
	uint32_t stream_pixel_format;
	/**
	 * フレームタイプ
	 */
	core::raw_frame_t stream_frame_type;
	/**
	 * フレームレート
	 */
	float stream_fps;
	/**
	 * 映像のデータバイト数
	 */
	size_t image_bytes;
	/**
	 * 映像受け取りバッファー配列
	 */
	buffer_t *m_buffers;
	/**
	 * 映像受け取りバッファー数
	 */
	uint32_t m_buffersNums;
	/**
	 * 映像取得スレッド
	 * async=trueの場合にhandle_frameを呼び出すワーカースレッド
	 */
	std::thread v4l2_thread;
	/**
	 * 対応しているコントロール機能のv4l2_queryctrl構造体マップ
	 */
	std::unordered_map<uint32_t, QueryCtrlSp> supported;

	/**
	 * 映像取得スレッドの実行関数
	 * @param buf_nums
	 */
	void v4l2_thread_func(const int &buf_nums);
	/**
	 * 映像取得ループ
	 * ワーカースレッド上で呼ばれる
	 * @return
	 */
	int v4l2_loop();
	/**
	 * ::stopの実体
	 * ::stopはIPipelineの純粋仮想関数なのでデストラクタから呼び出すのは良くないので実際の処理内部関数として分離
	 * @return
	 */
	int internal_stop();
	/**
	 * オープンしているudmabufやv4l2機器を閉じる
	*/
	int internal_close_locked();
	/**
	 * 映像ストリーム開始
	 * ワーカースレッド上で呼ばれる
	 * @return
	 */
	int start_stream_locked();
	/**
	 * 映像ストリーム終了
	 * ワーカースレッド上で呼ばれる
	 * @return
	 */
	int stop_stream_locked();
	/**
	 * v4l2からの映像取得の準備
	 * ワーカースレッド上で呼ばれる
	 * @param buf_nums
	 * @param width
	 * @param height
	 * @param pixel_format
	 * @return
	 */
	int init_v4l2_locked(
		const int &buf_nums,
	  	const uint32_t &width, const uint32_t &height,
	  	const uint32_t &pixel_format);
	/**
	 * v4l2からの映像取得終了処理
	 * ワーカースレッド上で呼ばれる
	 * @return
	 */
	int release_mmap_locked();
	/**
	 * m_buffersの破棄処理
	*/
	void internal_release_mmap_buffers();
	/**
	 * 画像データ読み込み用のメモリマップを初期化
	 * ワーカースレッド上で呼ばれる
	 * init_device_lockedの下請け
	 * @param buf_nums
	 */
	int init_mmap_locked(const int &buf_nums);
	/**
	 * udmabufを使うとき
	 * init_mmap_lockedの下請け
	 * @param req
	*/
	int init_mmap_locked_udmabuf(struct v4l2_requestbuffers &req);
	/**
	 * V4L2_MEMORY_DMABUFまたはV4L2_MEMORY_MMAPを使うとき
	 * init_mmap_lockedの下請け
	 * @param req
	*/
	int init_mmap_udmabuf_other(struct v4l2_requestbuffers &req);
	/**
	 * @brief 対応するピクセルフォーマット一覧を取得する
	 *        v4l2_lockをロックした状態で呼び出すこと
	 *
	 * @return std::vector 空vectorでなければこのvectorに含まれるものだけを返す,
	 *                     空ならV4L2機器がサポートするピクセルフォーマット全てを返す
	 */
	std::vector<uint32_t> get_supported_pixel_formats_locked(const std::vector<uint32_t> preffered = {}) const;
	/**
	 * 実際の解像度変更処理
	 * ワーカースレッド上で呼ばれる
	 * @param width
	 * @param height
	 * @param pixel_format V4L2_PIX_FMT_XXX
	 * @return
	 */
	int handle_resize(
		const uint32_t &width, const uint32_t &height,
		const uint32_t &pixel_format);
protected:
	mutable Mutex v4l2_lock;

	/**
	 * 実行中かどうかをセット
	 * @param is_running
	 * @return
	 */
	inline bool set_running(const bool &is_running) {
		const bool prev = m_running;
		m_running  = is_running;
		return (prev);
	};

	/**
	 * @brief 映像取得開始時の処理, 純粋仮想関数
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_start() = 0;
	/**
	 * @brief 映像取得終了時の処理, 純粋仮想関数
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_stop() = 0; 
	/**
	 * @brief 映像取得でエラーが発生したときの処理, 純粋仮想関数
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_error() = 0; 
	/**
	 * @brief 新しい映像を受け取ったときの処理, 純粋仮想関数
	 *        ワーカースレッド上で呼ばれる
	 *
	 * @param buf 共有メモリー情報
	 * @param bytes 映像データのサイズ
	 * @return int 負:エラー 0以上:読み込んだデータバイト数
	 */
	virtual int on_frame_ready(const buffer_t &buf, const size_t &bytes) = 0;

	/**
	 * 映像データの処理
	 * 映像データがないときはmax_wait_frame_usで指定した時間待機する
	 * ワーカースレッド上で呼ばれる
	 * @param max_wait_frame_us 最大映像待ち時間[マイクロ秒]
	 * @return 負:エラー 0以上:読み込んだデータバイト数
	 */
	int handle_frame(const suseconds_t &max_wait_frame_us = MAX_WAIT_FRAME_US);
	/**
	 * ctrl_idで指定したコントロール機能のQueryCtrlSpを返す
	 * 未対応のコントロール機能またはv4l2機器をオープンしていなければnullptrを返す
	 * @param ctrl_id
	 * @return
	 */
	QueryCtrlSp get_ctrl(const uint32_t &ctrl_id);
public:
	/**
	 * @brief コンストラクタ
	 *
	 * @param device_name v4l2機器名
	 * @param async 映像データの受取りを専用ワーカースレッドで行うかどうか
	 * @param udmabuf_name udmabufのデバイスファイル名
	 */
	V4l2SourceBase(std::string device_name, const bool &async = true, std::string udmabuf_name = "");
	/**
	 * @brief デストラクタ
	 *
	 */
	virtual ~V4l2SourceBase();

	/**
	 * 実行中かどうか
	 * @return
	 */
	inline const bool is_running() const { return m_running; };

	/**
	 * @brief ネゴシーエーションしたフレームタイプを取得する
	 * 
	 * @return core::raw_frame_t 
	 */
	inline core::raw_frame_t get_frame_type() const  { return stream_frame_type; };

	/**
	 * コンストラクタで指定したv4l2機器をオープン
	 * @return
	 */
	int open();
	/**
	 * ストリーム中であればストリーム終了したnoti
	 * v4l2機器をオープンしていればクローズする
	 * @return
	 */
	int close();

	/**
	 * コンストラクタで指定したv4l2機器をオープンして映像取得開始する
	 * @param buf_num キャプチャ用のバッファ数、デフォルトはDEFAULT_BUFFER_NUMS
	 * @return
	 */
	virtual int start(const int &buf_nums = DEFAULT_BUFFER_NUMS);
	/**
	 * 映像取得終了
	 * @return
	 */
	virtual int stop();

	/**
	 * @brief 対応するピクセルフォーマット一覧を取得する
	 *
	 * @return std::vector 空vectorでなければこのvectorに含まれるものだけを返す,
	 *                     空ならV4L2機器がサポートするピクセルフォーマット全てを返す
	 *
	 * @param preffered
	 * @return std::vector<uint32_t>
	 */
	inline std::vector<uint32_t> get_supported_pixel_formats(const std::vector<uint32_t> preffered = {}) const {
		AutoMutex lock(v4l2_lock);
		return get_supported_pixel_formats_locked(preffered);
	}

	/**
	 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
	 * @return
	 */
	std::string get_supported_size() const;

	/**
	 * 指定したピクセルフォーマント・解像度・フレームレートに対応しているかどうかを取得
	 * @param width
	 * @param height
	 * @param pixel_format V4L2_PIX_FMT_XXX, 省略時は前回に設定した値を使う
	 * @param min_fps 最小フレームレート, 省略時はDEFAULT_PREVIEW_FPS_MINを使う
	 * @param max_fps 最大フレームレート, 省略時はDEFAULT_PREVIEW_FPS_MAXを使う
	 * @return 0: 対応している, 0以外: 対応していない
	 */
	int find_stream(
		const uint32_t &width, const uint32_t &height,
		uint32_t pixel_format = 0,
		const float &min_fps = DEFAULT_PREVIEW_FPS_MIN, const float &max_fps = DEFAULT_PREVIEW_FPS_MAX);

	/**
	 * 映像サイズを変更
	 * @param width
	 * @param height
	 * @param pixel_format V4L2_PIX_FMT_XXX, 省略時は前回に設定した値を使う
	 * @return
	 */
	int resize(const uint32_t &width, const uint32_t &height, const uint32_t &pixel_format = 0);

	/**
	 * ctrl_idで指定したコントロール機能に対応しているかどうかを取得
	 * v4l2機器をオープンしているときのみ有効。closeしているときは常にfalseを返す
	 * @param ctrl_id
	 * @return
	 */
	bool is_ctrl_supported(const uint32_t &ctrl_id) override;
	/**
	 * ctrl_idで指定したコントロール機能の設定値を取得する
	 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
	 * @param ctrl_id
	 * @param value
	 * @return
	 */
	int get_ctrl_value(const uint32_t &ctrl_id, int32_t &value) override;
	/**
	 * ctrl_idで指定したコントロール機能へ値を設定する
	 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
	 * @param ctrl_id
	 * @param value
	 * @return
	 */
	int set_ctrl_value(const uint32_t &ctrl_id, const int32_t &value) override;
	/**
	 * 最小値/最大値/ステップ値/デフォルト値/現在値を取得する
	 * @param ctrl_id
	 * @param values
	 * @return
	 */
	int get_ctrl(const uint32_t &ctrl_id, uvc::control_value32_t &values) override;
	/**
	 * コントロール機能がメニュータイプの場合の設定項目値を取得する
	 * @param ctrl_id
	 * @param items 設定項目値をセットするstd::vector<std::string>
	 */
	int get_menu_items(const uint32_t &ctrl_id, std::vector<std::string> &items) override;
};

//--------------------------------------------------------------------------------
typedef std::function<int(const uint8_t *image, const size_t &bytes, const buffer_t &buffer)> OnFrameReadyFunc;
typedef std::function<void()> LifeCycletEventFunc;

/**
 * @brief フレームコールバック関数をセットして映像データを受け取るようにしたV4l2SourceBase実装
 * 
 */
class V4l2Source : virtual public V4l2SourceBase {
private:
	LifeCycletEventFunc on_start_callback;
	LifeCycletEventFunc on_stop_callback;
	LifeCycletEventFunc on_error_callback;
	OnFrameReadyFunc on_frame_ready_callbac;
protected:
	/**
	 * @brief 映像取得開始時の処理, V4l2SourceBaseの純粋仮想関数を実装
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_start() override;
	/**
	 * @brief 映像取得終了時の処理, V4l2SourceBaseの純粋仮想関数を実装
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_stop() override; 
	/**
	 * @brief 映像取得でエラーが発生したときの処理, 純粋仮想関数
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_error() override; 
	/**
	 * @brief 新しい映像を受け取ったときの処理, V4l2SourceBaseの純粋仮想関数を実装
	 *        ワーカースレッド上で呼ばれる
	 *
	 * @param buf 共有メモリー情報
	 * @param bytes 映像データのサイズ
	 * @return int 負:エラー 0以上:読み込んだデータバイト数
	 */
	virtual int on_frame_ready(const buffer_t &buf, const size_t &bytes) override;
public:
	/**
	 * @brief コンストラクタ
	 *
	 * @param device_name v4l2機器名
	 * @param async 映像データの受取りを専用ワーカースレッドで行うかどうか
	 * @param udmabuf_name udmabufのデバイスファイル名
	 */
	V4l2Source(std::string device_name, const bool &async = true, std::string udmabuf_name = "");
	/**
	 * @brief デストラクタ
	 *
	 */
	virtual ~V4l2Source();

	/**
	 * 映像データの処理
	 * 映像データがないときはmax_wait_frame_usで指定した時間待機する
	 * ワーカースレッド上で呼ばれる
	 * スコープをpublicに変更
	 * @param max_wait_frame_us 最大映像待ち時間[マイクロ秒]
	 * @return 負:エラー 0以上:読み込んだデータバイト数
	 */
	inline int handle_frame(const suseconds_t &max_wait_frame_us = MAX_WAIT_FRAME_US) {
		return V4l2SourceBase::handle_frame(max_wait_frame_us);
	}

	inline V4l2Source &set_on_start(LifeCycletEventFunc callback) {
		on_start_callback = callback;
		return *this;
	}
	inline V4l2Source &set_on_stop(LifeCycletEventFunc callback) {
		on_stop_callback = callback;
		return *this;
	}
	inline V4l2Source &set_on_error(LifeCycletEventFunc callback) {
		on_error_callback = callback;
		return *this;
	}
	/**
	 * @brief フレームコールバック関数をセット
	 * 
	 * @param callback 
	 * @return V4l2Source& 
	 */
	inline V4l2Source &set_on_frame_ready(OnFrameReadyFunc callback) {
		AutoMutex lock(v4l2_lock);
		on_frame_ready_callbac = callback;
		return *this;
	}
};

typedef std::unique_ptr<V4l2Source> V4l2SourceUp;
typedef std::shared_ptr<V4l2Source> V4l2SourceSp;

} // namespace serenegiant::v4l2

#endif  // AANDUSB_V4L2_SOURCE_H

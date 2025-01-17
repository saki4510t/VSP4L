/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_PIPELINE_V4L2_SOURCE_H
#define AANDUSB_PIPELINE_V4L2_SOURCE_H

#include <stddef.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

// common
#include "mutex.h"
#include "condition.h"
// pipeline
#include "pipeline/pipeline_base.h"
// v4l2
#include "v4l2/v4l2.h"

namespace usb = serenegiant::usb;
namespace uvc = serenegiant::usb::uvc;
namespace sere_pipeline = serenegiant::pipeline;

namespace serenegiant::v4l2::pipeline {

/**
 * v4l2からの映像をソースとして使うためのパイプライン(始点)
 * FIXME V4l2SourceBaseを使うように変更する
 */
class V4L2SourcePipeline : virtual public sere_pipeline::IPipeline {
private:
	const std::string device_name;
	mutable Mutex v4l2_lock;
	volatile int m_fd;
	/**
	 * v4l2の状態
	 */
	state_t m_state;
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
	 */
	std::thread v4l2_thread;
	/**
	 * 対応しているコントロール機能のv4l2_queryctrl構造体マップ
	 */
	std::unordered_map<uint32_t, QueryCtrlSp> supported;

	/**
	 * 映像取得スレッドの実行関数
	 */
	void v4l2_thread_func();
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
	 * @param width
	 * @param height
	 * @param pixel_format
	 * @return
	 */
	int init_v4l2_locked(
	  	const uint32_t &width, const uint32_t &height,
	  	const uint32_t &pixel_format);
	/**
	 * v4l2からの映像取得終了処理
	 * ワーカースレッド上で呼ばれる
	 * @return
	 */
	int release_mmap_locked(void);
	/**
	 * 画像データ読み込み用のメモリマップを初期化
	 * ワーカースレッド上で呼ばれる
	 * init_device_lockedの下請け
	 */
	int init_mmap_locked(void);
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
	/**
	 * 映像データを取得する
	 * 映像データがないときはMAX_WAIT_FRAME_USで指定した時間待機する
	 * ワーカースレッド上で呼ばれる
	 * @param dst
	 * @param capacity
	 * @return 負:エラー 0以上:読み込んだデータバイト数
	 */
	int wait_frame(uint8_t *dst, const size_t &capacity);
	/**
	 * 映像データを指定したバッファにコピーする
	 * ワーカースレッド上で呼ばれる
	 * @param dst
	 * @param capacity
	 * @return 負:エラー, 0以上:読み込んだ映像データのバイト数
	 */
	int read_frame(uint8_t *dst, const size_t &capacity);
protected:
	/**
	 * 映像取得開始時の処理
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_start();
	/**
	 * 映像取得終了時の処理
	 * ワーカースレッド上で呼ばれる
	 */
	virtual void on_stop();
	/**
	 * ctrl_idで指定したコントロール機能のQueryCtrlSpを返す
	 * 未対応のコントロール機能またはv4l2機器をオープンしていなければnullptrを返す
	 * @param ctrl_id
	 * @return
	 */
	QueryCtrlSp get_ctrl(const uint32_t &ctrl_id);
public:
	/**
	 * コンストラクタ
	 * @param device_name v4l2機器名
	 */
	V4L2SourcePipeline(std::string device_name);
	/**
	 * デストラクタ
	 */
	virtual ~V4L2SourcePipeline();

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
	 * IPipelineの純粋仮想関数
	 * @return
	 */
	virtual int start() override;
	/**
	 * 映像取得終了
	 * IPipelineの純粋仮想関数
	 * @return
	 */
	virtual int stop() override;

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
	 * IPipelineの純粋仮想関数
	 * @param frame
	 * @return
	 */
	virtual int queue_frame(core::BaseVideoFrame *frame) override;

	/**
	 * ctrl_idで指定したコントロール機能に対応しているかどうかを取得
	 * v4l2機器をオープンしているときのみ有効。closeしているときは常にfalseを返す
	 * @param ctrl_id
	 * @return
	 */
	bool is_ctrl_supported(const uint32_t &ctrl_id);
	/**
	 * ctrl_idで指定したコントロール機能の設定値を取得する
	 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
	 * @param ctrl_id
	 * @param value
	 * @return
	 */
	int get_ctrl_value(const uint32_t &ctrl_id, int32_t &value);
	/**
	 * ctrl_idで指定したコントロール機能へ値を設定する
	 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
	 * @param ctrl_id
	 * @param value
	 * @return
	 */
	int set_ctrl_value(const uint32_t &ctrl_id, const int32_t &value);
	/**
	 * 最小値/最大値/ステップ値/デフォルト値/現在値を取得する
	 * @param ctrl_id
	 * @param values
	 * @return
	 */
	int get_ctrl(const uint32_t &ctrl_id, uvc::control_value32_t &values);

	/**
	 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
	 * @param values
	 * @return
	 */
	int update_brightness(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
	 * @param value
	 * @return
	 */
	int set_brightness(const int32_t &value);
	/**
	 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
	 * @return
	 */
	int32_t get_brightness();
	
	/**
	 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
	 * @param values
	 * @return
	 */
	int update_contrast(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
	 * @param value
	 * @return
	 */
	int set_contrast(const int32_t &value);
	/**
	 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
	 * @return
	 */
	int32_t get_contrast();

	/**
	 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
	 * @param values
	 * @return
	 */
	int update_saturation(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
	 * @param value
	 * @return
	 */
	int set_saturation(const int32_t &value);
	/**
	 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
	 * @return
	 */
	int32_t get_saturation();

	/**
	 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
	 * @param values
	 * @return
	 */
	int update_hue(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
	 * @param value
	 * @return
	 */
	int set_hue(const int32_t &value);
	/**
	 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
	 * @return
	 */
	int32_t get_hue();

	/**
	 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
	 * @param values
	 * @return
	 */
	int update_auto_white_blance(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
	 * @param value
	 * @return
	 */
	int set_auto_white_blance(const int32_t &value);
	/**
	 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
	 * @return
	 */
	int32_t get_auto_white_blance();

	/**
	 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
	 * @param values
	 * @return
	 */
	int update_gamma(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
	 * @param value
	 * @return
	 */
	int set_gamma(const int32_t &value);
	/**
	 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
	 * @return
	 */
	int32_t get_gamma();

	/**
	 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
	 * @param values
	 * @return
	 */
	int update_auto_gain(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
	 * @param value
	 * @return
	 */
	int set_auto_gain(const int32_t &value);
	/**
	 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
	 * @return
	 */
	int32_t get_auto_gain();

	/**
	 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
	 * @param values
	 * @return
	 */
	int update_gain(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
	 * @param value
	 * @return
	 */
	int set_gain(const int32_t &value);
	/**
	 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
	 * @return
	 */
	int32_t get_gain();

	/**
	 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
	 * @param values
	 * @return
	 */
	int update_powerline_frequency(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
	 * @param value
	 * @return
	 */
	int set_powerline_frequency(const int32_t &value);
	/**
	 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
	 * @return
	 */
	int32_t get_powerline_frequency();

	/**
	 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
	 * @param values
	 * @return
	 */
	int update_auto_hue(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
	 * @param value
	 * @return
	 */
	int set_auto_hue(const int32_t &value);
	/**
	 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
	 * @return
	 */
	int32_t get_auto_hue();

	/**
	 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
	 * @param values
	 * @return
	 */
	int update_white_blance(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
	 * @param value
	 * @return
	 */
	int set_white_blance(const int32_t &value);
	/**
	 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
	 * @return
	 */
	int32_t get_white_blance();

	/**
	 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
	 * @param values
	 * @return
	 */
	int update_sharpness(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
	 * @param value
	 * @return
	 */
	int set_sharpness(const int32_t &value);
	/**
	 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
	 * @return
	 */
	int32_t get_sharpness();

	/**
	 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
	 * @param values
	 * @return
	 */
	int update_backlight_comp(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
	 * @param value
	 * @return
	 */
	int set_backlight_comp(const int32_t &value);
	/**
	 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
	 * @return
	 */
	int32_t get_backlight_comp();

	/**
	 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
	 * @param values
	 * @return
	 */
	int update_exposure_auto(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
	 * @param value
	 * @return
	 */
	int set_exposure_auto(const int32_t &value);
	/**
	 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
	 * @return
	 */
	int32_t get_exposure_auto();

	/**
	 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
	 * @param values
	 * @return
	 */
	int update_exposure_abs(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
	 * @param value
	 * @return
	 */
	int set_exposure_abs(const int32_t &value);
	/**
	 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
	 * @return
	 */
	int32_t get_exposure_abs();

	/**
	 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
	 * @param values
	 * @return
	 */
	int update_exposure_auto_priority(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
	 * @param value
	 * @return
	 */
	int set_exposure_auto_priority(const int32_t &value);
	/**
	 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
	 * @return
	 */
	int32_t get_exposure_auto_priority();

	/**
	 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
	 * @param values
	 * @return
	 */
	int update_pan_rel(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
	 * @param value
	 * @return
	 */
	int set_pan_rel(const int32_t &value);
	/**
	 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
	 * @return
	 */
	int32_t get_pane_rel();

	/**
	 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
	 * @param values
	 * @return
	 */
	int update_tilt_rel(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
	 * @param value
	 * @return
	 */
	int set_tilt_rel(const int32_t &value);
	/**
	 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
	 * @return
	 */
	int32_t get_tilt_rel();

	/**
	 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
	 * @param values
	 * @return
	 */
	int update_pan_abs(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
	 * @param value
	 * @return
	 */
	int set_pan_abs(const int32_t &value);
	/**
	 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
	 * @return
	 */
	int32_t get_pan_abs();

	/**
	 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
	 * @param values
	 * @return
	 */
	int update_tilt_abs(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
	 * @param value
	 * @return
	 */
	int set_tilt_abs(const int32_t &value);
	/**
	 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
	 * @return
	 */
	int32_t get_tilt_abs();

	/**
	 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
	 * @param values
	 * @return
	 */
	int update_focus_abs(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
	 * @param value
	 * @return
	 */
	int set_focus_abs(const int32_t &value);
	/**
	 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
	 * @return
	 */
	int32_t get_focus_abs();

	/**
	 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
	 * @param values
	 * @return
	 */
	int update_focus_rel(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
	 * @param value
	 * @return
	 */
	int set_focus_rel(const int32_t &value);
	/**
	 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
	 * @return
	 */
	int32_t get_focus_rel();

	/**
	 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
	 * @param values
	 * @return
	 */
	int update_focus_auto(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
	 * @param value
	 * @return
	 */
	int set_focus_auto(const int32_t &value);
	/**
	 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
	 * @return
	 */
	int32_t get_focus_auto();

	/**
	 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
	 * @param values
	 * @return
	 */
	int update_zoom_abs(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
	 * @param value
	 * @return
	 */
	int set_zoom_abs(const int32_t &value);
	/**
	 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
	 * @return
	 */
	int32_t get_zoom_abs();

	/**
	 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
	 * @param values
	 * @return
	 */
	int update_zoom_rel(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
	 * @param value
	 * @return
	 */
	int set_zoom_rel(const int32_t &value);
	/**
	 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
	 * @return
	 */
	int32_t get_zoom_rel();

	/**
	 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
	 * @param values
	 * @return
	 */
	int update_zoom_continuous(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
	 * @param value
	 * @return
	 */
	int set_zoom_continuous(const int32_t &value);
	/**
	 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
	 * @return
	 */
	int32_t get_zoom_continuous();

	/**
	 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
	 * @param values
	 * @return
	 */
	int update_privacy(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
	 * @param value
	 * @return
	 */
	int set_privacy(const int32_t &value);
	/**
	 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
	 * @return
	 */
	int32_t get_privacy();

	/**
	 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
	 * @param values
	 * @return
	 */
	int update_iris_abs(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
	 * @param value
	 * @return
	 */
	int set_iris_abs(const int32_t &value);
	/**
	 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
	 * @return
	 */
	int32_t get_iris_abs();

	/**
	 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
	 * @param values
	 * @return
	 */
	int update_iris_rel(uvc::control_value32_t &values);
	/**
	 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
	 * @param value
	 * @return
	 */
	int set_iris_rel(const int32_t &value);
	/**
	 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
	 * @return
	 */
	int32_t get_iris_rel();

};

typedef std::unique_ptr<V4L2SourcePipeline> V4L2SourcePipelineUp;
typedef std::shared_ptr<V4L2SourcePipeline> V4L2SourcePipelineSp;

}	// namespace serenegiant::v4l2::pipeline

#endif //AANDUSB_PIPELINE_V4L2_SOURCE_H

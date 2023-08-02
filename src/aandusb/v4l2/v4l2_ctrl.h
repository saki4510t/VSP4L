/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_V4L2_CTRL_H
#define AANDUSB_V4L2_CTRL_H

// v4l2
#include "v4l2/v4l2.h"

namespace uvc = serenegiant::usb::uvc;

namespace serenegiant::v4l2 {

class V4L2Ctrl {
protected:
    V4L2Ctrl();
    virtual ~V4L2Ctrl();
public:
	/**
	 * ctrl_idで指定したコントロール機能に対応しているかどうかを取得
	 * v4l2機器をオープンしているときのみ有効。closeしているときは常にfalseを返す
	 * @param ctrl_id
	 * @return
	 */
	virtual bool is_ctrl_supported(const uint32_t &ctrl_id) = 0;
	/**
	 * ctrl_idで指定したコントロール機能の設定値を取得する
	 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
	 * @param ctrl_id
	 * @param value
     * @return 0: 正常終了
	 */
	virtual int get_ctrl_value(const uint32_t &ctrl_id, int32_t &value) = 0;
	/**
	 * ctrl_idで指定したコントロール機能へ値を設定する
	 * v4l2機器をオープンしているとき&対応しているコントロールの場合のみ有効
	 * @param ctrl_id
	 * @param value
     * @return 0: 正常終了
	 */
	virtual int set_ctrl_value(const uint32_t &ctrl_id, const int32_t &value) = 0;
	/**
	 * 最小値/最大値/ステップ値/デフォルト値/現在値を取得する
	 * @param ctrl_id
	 * @param values
     * @return 0: 正常終了
	 */
	virtual int get_ctrl(const uint32_t &ctrl_id, uvc::control_value32_t &value) = 0;
	/**
	 * コントロール機能がメニュータイプの場合の設定項目値を取得する
	 * @param ctrl_id
	 * @param items 設定項目値をセットするstd::vector<std::string>
	 */
	virtual int get_menu_items(const uint32_t &ctrl_id, std::vector<std::string> &items) = 0;

    /**
     * 設定値を適用する
     * @param value
     * @return 0: 正常終了
    */
    int set_ctrl(const uvc::control_value32_t &value);  
	//--------------------------------------------------------------------------------
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

}   // namespace serenegiant::v4l2

#endif  // AANDUSB_V4L2_CTRL_H

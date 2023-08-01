/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1    // set 0 if you need debug log, otherwise set 1
	#ifndef LOG_NDEBUG
	#define LOG_NDEBUG
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
	#define DEBUG_EGL_CHECK			// EGL関数のデバッグメッセージを表示するとき
#endif

#define LOG_TAG "V4L2Ctrl"

#include "utilbase.h"

// v4l2
#include "v4l2_ctrl.h"

namespace serenegiant::v4l2 {

V4L2Ctrl::V4L2Ctrl() {
    ENTER();
    EXIT();
}

V4L2Ctrl::~V4L2Ctrl() {
    ENTER();
    EXIT();
}

/**
 * 設定値を適用する
 * @param value
 * @return 0: 正常終了
*/
int V4L2Ctrl::set_ctrl(const uvc::control_value32_t &value) {
    ENTER();

    int result = core::USB_ERROR_NOT_SUPPORTED;

    if (is_ctrl_supported(value.id)) {
        result = set_ctrl_value(value.id, value.current);
    }

    RETURN(result, int);
}

//--------------------------------------------------------------------------------
// v4l2標準コントロール
/**
 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
 * @param values
 * @return
 */
int V4L2Ctrl::update_brightness(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_BRIGHTNESS, values);
}

/**
 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
 * @param value
 * @return
 */
int V4L2Ctrl::set_brightness(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_BRIGHTNESS, value);
}

/**
 * V4L2_CID_BRIGHTNESS (V4L2_CID_BASE + 0)
 * @return
 */
int32_t V4L2Ctrl::get_brightness() {
	int32_t result;
	get_ctrl_value(V4L2_CID_BRIGHTNESS, result);
	return result;
}

/**
 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
 * @param values
 * @return
 */
int V4L2Ctrl::update_contrast(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_CONTRAST, values);
}

/**
 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
 * @param value
 * @return
 */
int V4L2Ctrl::set_contrast(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_CONTRAST, value);
}

/**
 * V4L2_CID_CONTRAST (V4L2_CID_BASE + 1)
 * @return
 */
int32_t V4L2Ctrl::get_contrast() {
	int32_t result;
	get_ctrl_value(V4L2_CID_CONTRAST, result);
	return result;
}

/**
 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
 * @param values
 * @return
 */
int V4L2Ctrl::update_saturation(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_SATURATION, values);
}

/**
 * V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
 * @param value
 * @return
 */
int V4L2Ctrl::set_saturation(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_SATURATION, value);
}

/**
 *V4L2_CID_SATURATION (V4L2_CID_BASE + 2)
 * @return
 */
int32_t V4L2Ctrl::get_saturation() {
	int32_t result;
	get_ctrl_value(V4L2_CID_SATURATION, result);
	return result;
}

/**
 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
 * @param values
 * @return
 */
int V4L2Ctrl::update_hue(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_HUE, values);
}

/**
 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
 * @param value
 * @return
 */
int V4L2Ctrl::set_hue(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_HUE, value);
}

/**
 * V4L2_CID_HUE (V4L2_CID_BASE + 3)
 * @return
 */
int32_t V4L2Ctrl::get_hue() {
	int32_t result;
	get_ctrl_value(V4L2_CID_HUE, result);
	return result;
}

//#define V4L2_CID_AUDIO_VOLUME (V4L2_CID_BASE + 5)
//#define V4L2_CID_AUDIO_BALANCE (V4L2_CID_BASE + 6)
//#define V4L2_CID_AUDIO_BASS (V4L2_CID_BASE + 7)
//#define V4L2_CID_AUDIO_TREBLE (V4L2_CID_BASE + 8)
//#define V4L2_CID_AUDIO_MUTE (V4L2_CID_BASE + 9)
//#define V4L2_CID_AUDIO_LOUDNESS (V4L2_CID_BASE + 10)
//#define V4L2_CID_BLACK_LEVEL (V4L2_CID_BASE + 11)

/**
 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
 * @param values
 * @return
 */
int V4L2Ctrl::update_auto_white_blance(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_AUTO_WHITE_BALANCE, values);
}

/**
 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
 * @param value
 * @return
 */
int V4L2Ctrl::set_auto_white_blance(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_AUTO_WHITE_BALANCE, value);
}

/**
 * V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE + 12)
 * @return
 */
int32_t V4L2Ctrl::get_auto_white_blance() {
	int32_t result;
	get_ctrl_value(V4L2_CID_AUTO_WHITE_BALANCE, result);
	return result;
}

//#define V4L2_CID_DO_WHITE_BALANCE (V4L2_CID_BASE + 13)
//#define V4L2_CID_RED_BALANCE (V4L2_CID_BASE + 14)
//#define V4L2_CID_BLUE_BALANCE (V4L2_CID_BASE + 15)

/**
 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
 * @param values
 * @return
 */
int V4L2Ctrl::update_gamma(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_GAMMA, values);
}

/**
 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
 * @param value
 * @return
 */
int V4L2Ctrl::set_gamma(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_GAMMA, value);
}

/**
 * V4L2_CID_GAMMA (V4L2_CID_BASE + 16)
 * @return
 */
int32_t V4L2Ctrl::get_gamma() {
	int32_t result;
	get_ctrl_value(V4L2_CID_GAMMA, result);
	return result;
}


//#define V4L2_CID_WHITENESS (V4L2_CID_GAMMA)
//#define V4L2_CID_EXPOSURE (V4L2_CID_BASE + 17)
//#define

/**
 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
 * @param values
 * @return
 */
int V4L2Ctrl::update_auto_gain(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_AUTOGAIN, values);
}

/**
 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
 * @param value
 * @return
 */
int V4L2Ctrl::set_auto_gain(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_AUTOGAIN, value);
}

/**
 * V4L2_CID_AUTOGAIN (V4L2_CID_BASE + 18)
 * @return
 */
int32_t V4L2Ctrl::get_auto_gain() {
	int32_t result;
	get_ctrl_value(V4L2_CID_AUTOGAIN, result);
	return result;
}

/**
 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
 * @param values
 * @return
 */
int V4L2Ctrl::update_gain(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_GAIN, values);
}

/**
 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
 * @param value
 * @return
 */
int V4L2Ctrl::set_gain(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_GAIN, value);
}

/**
 * V4L2_CID_GAIN (V4L2_CID_BASE + 19)
 * @return
 */
int32_t V4L2Ctrl::get_gain() {
	int32_t result;
	get_ctrl_value(V4L2_CID_GAIN, result);
	return result;
}

//#define V4L2_CID_HFLIP (V4L2_CID_BASE + 20)
//#define V4L2_CID_VFLIP (V4L2_CID_BASE + 21)

/**
 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
 * @param values
 * @return
 */
int V4L2Ctrl::update_powerline_frequency(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_POWER_LINE_FREQUENCY, values);
}

/**
 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
 * @param value
 * @return
 */
int V4L2Ctrl::set_powerline_frequency(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_POWER_LINE_FREQUENCY, value);
}

/**
 * V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_BASE + 24)
 * @return
 */
int32_t V4L2Ctrl::get_powerline_frequency() {
	int32_t result;
	get_ctrl_value(V4L2_CID_POWER_LINE_FREQUENCY, result);
	return result;
}

/**
 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
 * @param values
 * @return
 */
int V4L2Ctrl::update_auto_hue(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_HUE_AUTO, values);
}

/**
 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
 * @param value
 * @return
 */
int V4L2Ctrl::set_auto_hue(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_HUE_AUTO, value);
}

/**
 * V4L2_CID_HUE_AUTO (V4L2_CID_BASE + 25)
 * @return
 */
int32_t V4L2Ctrl::get_auto_hue() {
	int32_t result;
	get_ctrl_value(V4L2_CID_HUE_AUTO, result);
	return result;
}

/**
 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
 * @param values
 * @return
 */
int V4L2Ctrl::update_white_blance(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_WHITE_BALANCE_TEMPERATURE, values);
}

/**
 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
 * @param value
 * @return
 */
int V4L2Ctrl::set_white_blance(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

/**
 * V4L2_CID_WHITE_BALANCE_TEMPERATURE (V4L2_CID_BASE + 26)
 * @return
 */
int32_t V4L2Ctrl::get_white_blance() {
	int32_t result;
	get_ctrl_value(V4L2_CID_WHITE_BALANCE_TEMPERATURE, result);
	return result;
}

/**
 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
 * @param values
 * @return
 */
int V4L2Ctrl::update_sharpness(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_SHARPNESS, values);
}

/**
 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
 * @param value
 * @return
 */
int V4L2Ctrl::set_sharpness(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_SHARPNESS, value);
}

/**
 * V4L2_CID_SHARPNESS (V4L2_CID_BASE + 27)
 * @return
 */
int32_t V4L2Ctrl::get_sharpness() {
	int32_t result;
	get_ctrl_value(V4L2_CID_SHARPNESS, result);
	return result;
}

/**
 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
 * @param values
 * @return
 */
int V4L2Ctrl::update_backlight_comp(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_BACKLIGHT_COMPENSATION, values);
}

/**
 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
 * @param value
 * @return
 */
int V4L2Ctrl::set_backlight_comp(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_BACKLIGHT_COMPENSATION, value);
}

/**
 * V4L2_CID_BACKLIGHT_COMPENSATION (V4L2_CID_BASE + 28)
 * @return
 */
int32_t V4L2Ctrl::get_backlight_comp() {
	int32_t result;
	get_ctrl_value(V4L2_CID_BACKLIGHT_COMPENSATION, result);
	return result;
}

//#define V4L2_CID_CHROMA_AGC (V4L2_CID_BASE + 29)
//#define V4L2_CID_COLOR_KILLER (V4L2_CID_BASE + 30)
//#define V4L2_CID_COLORFX (V4L2_CID_BASE + 31)
//#define V4L2_CID_AUTOBRIGHTNESS (V4L2_CID_BASE + 32)
//#define V4L2_CID_BAND_STOP_FILTER (V4L2_CID_BASE + 33)
//#define V4L2_CID_ROTATE (V4L2_CID_BASE + 34)
//#define V4L2_CID_BG_COLOR (V4L2_CID_BASE + 35)
//#define V4L2_CID_CHROMA_GAIN (V4L2_CID_BASE + 36)
//#define V4L2_CID_ILLUMINATORS_1 (V4L2_CID_BASE + 37)
//#define V4L2_CID_ILLUMINATORS_2 (V4L2_CID_BASE + 38)
//#define V4L2_CID_MIN_BUFFERS_FOR_CAPTURE (V4L2_CID_BASE + 39)
//#define V4L2_CID_MIN_BUFFERS_FOR_OUTPUT (V4L2_CID_BASE + 40)
//#define V4L2_CID_ALPHA_COMPONENT (V4L2_CID_BASE + 41)
//#define V4L2_CID_COLORFX_CBCR (V4L2_CID_BASE + 42)

// v4l2標準カメラコントロール
/**
 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
 * @param values
 * @return
 */
int V4L2Ctrl::update_exposure_auto(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_EXPOSURE_AUTO, values);
}

/**
 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
 * @param value
 * @return
 */
int V4L2Ctrl::set_exposure_auto(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_EXPOSURE_AUTO, value);
}

/**
 * V4L2_CID_EXPOSURE_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 1)
 * @return
 */
int32_t V4L2Ctrl::get_exposure_auto() {
	int32_t result;
	get_ctrl_value(V4L2_CID_EXPOSURE_AUTO, result);
	return result;
}

/**
 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
 * @param values
 * @return
 */
int V4L2Ctrl::update_exposure_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_EXPOSURE_ABSOLUTE, values);
}

/**
 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
 * @param value
 * @return
 */
int V4L2Ctrl::set_exposure_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

/**
 * V4L2_CID_EXPOSURE_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 2)
 * @return
 */
int32_t V4L2Ctrl::get_exposure_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_EXPOSURE_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
 * @param values
 * @return
 */
int V4L2Ctrl::update_exposure_auto_priority(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_EXPOSURE_AUTO_PRIORITY, values);
}

/**
 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
 * @param value
 * @return
 */
int V4L2Ctrl::set_exposure_auto_priority(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_EXPOSURE_AUTO_PRIORITY, value);
}

/**
 * V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE + 3)
 * @return
 */
int32_t V4L2Ctrl::get_exposure_auto_priority() {
	int32_t result;
	get_ctrl_value(V4L2_CID_EXPOSURE_AUTO_PRIORITY, result);
	return result;
}

/**
 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
 * @param values
 * @return
 */
int V4L2Ctrl::update_pan_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_PAN_RELATIVE, values);
}

/**
 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
 * @param value
 * @return
 */
int V4L2Ctrl::set_pan_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_PAN_RELATIVE, value);
}

/**
 * V4L2_CID_PAN_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 4)
 * @return
 */
int32_t V4L2Ctrl::get_pane_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_PAN_RELATIVE, result);
	return result;
}

/**
 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
 * @param values
 * @return
 */
int V4L2Ctrl::update_tilt_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_TILT_RELATIVE, values);
}

/**
 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
 * @param value
 * @return
 */
int V4L2Ctrl::set_tilt_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_TILT_RELATIVE, value);
}

/**
 * V4L2_CID_TILT_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 5)
 * @return
 */
int32_t V4L2Ctrl::get_tilt_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_TILT_RELATIVE, result);
	return result;
}

//#define V4L2_CID_PAN_RESET (V4L2_CID_CAMERA_CLASS_BASE + 6)
//#define V4L2_CID_TILT_RESET (V4L2_CID_CAMERA_CLASS_BASE + 7)

/**
 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
 * @param values
 * @return
 */
int V4L2Ctrl::update_pan_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_PAN_ABSOLUTE, values);
}

/**
 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
 * @param value
 * @return
 */
int V4L2Ctrl::set_pan_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_PAN_ABSOLUTE, value);
}

/**
 * V4L2_CID_PAN_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 8)
 * @return
 */
int32_t V4L2Ctrl::get_pan_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_PAN_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
 * @param values
 * @return
 */
int V4L2Ctrl::update_tilt_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_TILT_ABSOLUTE, values);
}

/**
 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
 * @param value
 * @return
 */
int V4L2Ctrl::set_tilt_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_TILT_ABSOLUTE, value);
}

/**
 * V4L2_CID_TILT_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 9)
 * @return
 */
int32_t V4L2Ctrl::get_tilt_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_TILT_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
 * @param values
 * @return
 */
int V4L2Ctrl::update_focus_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_FOCUS_ABSOLUTE, values);
}

/**
 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
 * @param value
 * @return
 */
int V4L2Ctrl::set_focus_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_FOCUS_ABSOLUTE, value);
}

/**
 * V4L2_CID_FOCUS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 10)
 * @return
 */
int32_t V4L2Ctrl::get_focus_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_FOCUS_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
 * @param values
 * @return
 */
int V4L2Ctrl::update_focus_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_FOCUS_RELATIVE, values);
}

/**
 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
 * @param value
 * @return
 */
int V4L2Ctrl::set_focus_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_FOCUS_RELATIVE, value);
}

/**
 * V4L2_CID_FOCUS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 11)
 * @return
 */
int32_t V4L2Ctrl::get_focus_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_FOCUS_RELATIVE, result);
	return result;
}

/**
 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
 * @param values
 * @return
 */
int V4L2Ctrl::update_focus_auto(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_FOCUS_AUTO, values);
}

/**
 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
 * @param value
 * @return
 */
int V4L2Ctrl::set_focus_auto(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_FOCUS_AUTO, value);
}

/**
 * V4L2_CID_FOCUS_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 12)
 * @return
 */
int32_t V4L2Ctrl::get_focus_auto() {
	int32_t result;
	get_ctrl_value(V4L2_CID_FOCUS_AUTO, result);
	return result;
}

/**
 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
 * @param values
 * @return
 */
int V4L2Ctrl::update_zoom_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_ZOOM_ABSOLUTE, values);
}

/**
 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
 * @param value
 * @return
 */
int V4L2Ctrl::set_zoom_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_ZOOM_ABSOLUTE, value);
}

/**
 * V4L2_CID_ZOOM_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 13)
 * @return
 */
int32_t V4L2Ctrl::get_zoom_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_ZOOM_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
 * @param values
 * @return
 */
int V4L2Ctrl::update_zoom_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_ZOOM_RELATIVE, values);
}

/**
 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
 * @param value
 * @return
 */
int V4L2Ctrl::set_zoom_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_ZOOM_RELATIVE, value);
}

/**
 * V4L2_CID_ZOOM_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 14)
 * @return
 */
int32_t V4L2Ctrl::get_zoom_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_ZOOM_RELATIVE, result);
	return result;
}

/**
 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
 * @param values
 * @return
 */
int V4L2Ctrl::update_zoom_continuous(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_ZOOM_CONTINUOUS, values);
}

/**
 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
 * @param value
 * @return
 */
int V4L2Ctrl::set_zoom_continuous(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_ZOOM_CONTINUOUS, value);
}

/**
 * V4L2_CID_ZOOM_CONTINUOUS (V4L2_CID_CAMERA_CLASS_BASE + 15)
 * @return
 */
int32_t V4L2Ctrl::get_zoom_continuous() {
	int32_t result;
	get_ctrl_value(V4L2_CID_ZOOM_CONTINUOUS, result);
	return result;
}

/**
 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
 * @param values
 * @return
 */
int V4L2Ctrl::update_privacy(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_PRIVACY, values);
}

/**
 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
 * @param value
 * @return
 */
int V4L2Ctrl::set_privacy(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_PRIVACY, value);
}

/**
 * V4L2_CID_PRIVACY (V4L2_CID_CAMERA_CLASS_BASE + 16)
 * @return
 */
int32_t V4L2Ctrl::get_privacy() {
	int32_t result;
	get_ctrl_value(V4L2_CID_PRIVACY, result);
	return result;
}

/**
 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
 * @param values
 * @return
 */
int V4L2Ctrl::update_iris_abs(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_IRIS_ABSOLUTE, values);
}

/**
 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
 * @param value
 * @return
 */
int V4L2Ctrl::set_iris_abs(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_IRIS_ABSOLUTE, value);
}

/**
 * V4L2_CID_IRIS_ABSOLUTE (V4L2_CID_CAMERA_CLASS_BASE + 17)
 * @return
 */
int32_t V4L2Ctrl::get_iris_abs() {
	int32_t result;
	get_ctrl_value(V4L2_CID_IRIS_ABSOLUTE, result);
	return result;
}

/**
 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
 * @param values
 * @return
 */
int V4L2Ctrl::update_iris_rel(uvc::control_value32_t &values) {
	return get_ctrl(V4L2_CID_IRIS_RELATIVE, values);
}

/**
 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
 * @param value
 * @return
 */
int V4L2Ctrl::set_iris_rel(const int32_t &value) {
	return set_ctrl_value(V4L2_CID_IRIS_RELATIVE, value);
}

/**
 * V4L2_CID_IRIS_RELATIVE (V4L2_CID_CAMERA_CLASS_BASE + 18)
 * @return
 */
int32_t V4L2Ctrl::get_iris_rel() {
	int32_t result;
	get_ctrl_value(V4L2_CID_IRIS_RELATIVE, result);
	return result;
}

//#define V4L2_CID_AUTO_EXPOSURE_BIAS (V4L2_CID_CAMERA_CLASS_BASE + 19)
//#define V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE (V4L2_CID_CAMERA_CLASS_BASE + 20)
//#define V4L2_CID_WIDE_DYNAMIC_RANGE (V4L2_CID_CAMERA_CLASS_BASE + 21)
//#define V4L2_CID_IMAGE_STABILIZATION (V4L2_CID_CAMERA_CLASS_BASE + 22)
//#define V4L2_CID_ISO_SENSITIVITY (V4L2_CID_CAMERA_CLASS_BASE + 23)
//#define V4L2_CID_ISO_SENSITIVITY_AUTO (V4L2_CID_CAMERA_CLASS_BASE + 24)
//#define V4L2_CID_EXPOSURE_METERING (V4L2_CID_CAMERA_CLASS_BASE + 25)
//#define V4L2_CID_SCENE_MODE (V4L2_CID_CAMERA_CLASS_BASE + 26)
//#define V4L2_CID_3A_LOCK (V4L2_CID_CAMERA_CLASS_BASE + 27)
//#define V4L2_LOCK_EXPOSURE (1 << 0)
//#define V4L2_LOCK_WHITE_BALANCE (1 << 1)
//#define V4L2_LOCK_FOCUS (1 << 2)
//#define V4L2_CID_AUTO_FOCUS_START (V4L2_CID_CAMERA_CLASS_BASE + 28)
//#define V4L2_CID_AUTO_FOCUS_STOP (V4L2_CID_CAMERA_CLASS_BASE + 29)
//#define V4L2_CID_AUTO_FOCUS_STATUS (V4L2_CID_CAMERA_CLASS_BASE + 30)
//#define V4L2_AUTO_FOCUS_STATUS_IDLE (0 << 0)
//#define V4L2_AUTO_FOCUS_STATUS_BUSY (1 << 0)
//#define V4L2_AUTO_FOCUS_STATUS_REACHED (1 << 1)
//#define V4L2_AUTO_FOCUS_STATUS_FAILED (1 << 2)
//#define V4L2_CID_AUTO_FOCUS_RANGE (V4L2_CID_CAMERA_CLASS_BASE + 31)
//#define V4L2_CID_PAN_SPEED (V4L2_CID_CAMERA_CLASS_BASE + 32)
//#define V4L2_CID_TILT_SPEED (V4L2_CID_CAMERA_CLASS_BASE + 33)
//#define V4L2_CID_CAMERA_ORIENTATION (V4L2_CID_CAMERA_CLASS_BASE + 34)
//#define V4L2_CAMERA_ORIENTATION_FRONT 0
//#define V4L2_CAMERA_ORIENTATION_BACK 1
//#define V4L2_CAMERA_ORIENTATION_EXTERNAL 2
//#define V4L2_CID_CAMERA_SENSOR_ROTATION (V4L2_CID_CAMERA_CLASS_BASE + 35)

}   // namespace serenegiant::v4l2

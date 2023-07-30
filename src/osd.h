#ifndef OSD_H_
#define OSD_H_

#include <functional>
#include <unordered_map>

#include "v4l2/v4l2_source.h"

#include "const.h"
#include "key_event.h"
#include "settings.h"

namespace serenegiant::app {

enum {
	PAGE_VERSION = 0,
	PAGE_SETTINGS_1,
	PAGE_ADJUST_1,
	PAGE_ADJUST_2,
	PAGE_ADJUST_3,
	PAGE_NUM,
} osd_page_t;

typedef struct _osd_value {
	uint32_t id;
	bool supported;
	bool enabled;
	bool modified;
	int32_t prev;
	uvc::control_value32_t value;
} osd_value_t;

typedef std::unique_ptr<osd_value_t> OSDValueUp;
typedef std::function<void(const bool &/*changed*/)> OnOSDCloseFunc;
typedef std::function<void(const CameraSettings &/*settings*/)> OnCameraSettingsChanged;

class OSD {
private:
	int page;
	AppSettings app_settings;
	CameraSettings camera_settings;

	OnOSDCloseFunc on_osd_close;
	OnCameraSettingsChanged on_camera_settings_changed;

	std::unordered_map<uint32_t, OSDValueUp> values;

	/**
	 * @brief 保存して閉じる
	 * 
	 */
	void save();
	/**
	 * @brief 保存せずに閉じる
	 * 
	 */
	void cancel();
	/**
	 * @brief 前のーページへ移動
	 * 
	 */
	void prev();
	/**
	 * @brief 次のページへ移動
	 * 
	 */
	void next();

	/**
	 * @brief 機器情報画面
	*/
	void draw_version();
	/**
	 * @brief 設定画面1
	 * 
	 */
	void draw_settings_1();
	/**
	 * @brief 調整画面1
	 * 
	 */
	void draw_adjust_1();
	/**
	 * @brief 調整画面2
	 * 
	 */
	void draw_adjust_2();
	/**
	 * @brief 調整画面3
	 * 
	 */
	void draw_adjust_3();

	/**
	 * @brief デフォルトのボタン幅を取得
	 * 
	 * @return float 
	 */
	float get_button_width();
	/**
	 * @brief デフォルトのボタンを描画する
	 * 
	 */
	void draw_buttons_default();
	/**
	 * 指定したidのカメラコントロールに対応している場合に指定したラベルを
	 * ImGui::LabelTextとして表示する。
	 * 対応していない場合は空のImGui::LabelTextを表示する。
	 * @param id
	 * @param label
	*/
	void show_label(const uint32_t &id, const char *label);
	/**
	 * 指定したidのカメラコントロールに対応している場合に指定したラベルを持つ
	 * ImGui::SliderIntを表示する。
	 * 対応していない場合は空のImGui::LabelTextを表示する。
	 * @param id
	 * @param label
	*/
	void show_slider(const uint32_t &id, const char *label);
	/**
	 * 設定値が変更されたときの処理
	 * @param value
	*/
	void value_changed(const osd_value_t &value);
protected:
public:
	/**
	 * @brief コンストラクタ
	 *
	 */
	OSD();
	/**
	 * @brief デストラクタ
	 *
	 */
	~OSD();

	/**
	 * @brief OSD表示の準備
	 * @param source
	 * 
	 */
	void prepare(v4l2::V4l2SourceUp &source);

	/**
	 * @brief OSD表示中のキー入力
	 *
	 * @param event
	 */
	void on_key(const KeyEvent &event);

	/**
	 * @brief OSD表示の描画処理
	 *
	 */
	void draw(ImFont *large_font);

	inline OSD &set_on_osd_close(OnOSDCloseFunc callback) {
		on_osd_close = callback;
		return *this;
	}
	inline OSD &set_on_camera_settings_changed(OnCameraSettingsChanged callback) {
		on_camera_settings_changed = callback;
		return *this;
	}
};

}   // namespace serenegiant::app

#endif  // OSD_H_

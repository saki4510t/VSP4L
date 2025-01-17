// Vision Support Program for Linux ... EyeApp
// Copyright (C) 2023-2024 saki t_saki@serenegiant.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
#endif

#include "utilbase.h"

// common
#include "charutils.h"
// aandusb/v4l2
#include "v4l2/v4l2.h"
// app
#include "const.h"
#include "osd.h"

namespace serenegiant::app {

// OSDモード表示中の背景アルファ
#define OSD_BK_ALPHA (0.5f)
// OSDモード表示中の外周パディング
#define OSD_FRAME_PADDING (30)
// デフォルトの表示ページ
#define DEFAULE_PAGE (PAGE_SETTINGS_1)

static const char *APP_LABEL_RESTORE_SETTINGS = "RESTORE_SETTINGS";
// 調整1のラベル文字列
static const char *V4L2_LABEL_BRIGHTNESS = "BRIGHTNESS";
static const char *V4L2_LABEL_HUE = "HUE";
static const char *V4L2_LABEL_SATURATION = "SATURATION";
static const char *V4L2_LABEL_CONTRAST = "CONTRAST";
static const char *V4L2_LABEL_SHARPNESS = "SHARPNESS";
static const char *V4L2_LABEL_GAMMA = "GAMMA";
// 調整2のラベル文字列
static const char *V4L2_LABEL_DENOISE = "DENOISE";
static const char *V4L2_LABEL_AUTOGAIN = "AGC";
static const char *V4L2_LABEL_GAIN = "GAIN";
static const char *V4L2_LABEL_EXPOSURE_AUTO = "AE";
static const char *V4L2_LABEL_EXPOSURE = "EXPOSURE";
static const char *V4L2_LABEL_AUTO_N_PRESET_WHITE_BLANCE = "AWB";
// 調整3のラベル文字列
static const char *V4L2_LABEL_POWER_LINE_FREQUENCY = "PLF";
static const char *V4L2_LABEL_ZOOM_ABSOLUTE = "ZOOM";

//--------------------------------------------------------------------------------
/**
 * @brief コンストラクタ
 *
 */
/*public*/
OSD::OSD()
:	page(DEFAULE_PAGE),
	app_settings(),
	camera_modified(false)
{
	ENTER();
	EXIT();
}

/**
 * @brief デストラクタ
 *
 */
/*public*/
OSD::~OSD() {
	ENTER();
	EXIT();
}

/**
 * @brief OSD表示の準備
 * @param source
 * 
 */
/*public*/
void OSD::prepare(v4l2::V4l2SourceUp &source) {
	ENTER();

	// 表示ページをデフォルトページへ
	page = DEFAULE_PAGE;
	camera_modified = false;
	// 設定値を読み込む等
	app_settings.load();
	// カメラ設定を読み込む
	values.clear();
	for (const auto id: SUPPORTED_CTRLS) {
		if (id) {
			const auto supported = source->is_ctrl_supported(id);
			osd_value_t val = {
				.id = id,
				.supported = supported,
				.enabled = supported,
				.modified = false
			};
			if (supported) {
				source->get_ctrl(id, val.value);
				val.prev = val.value.current;
				const auto type = val.value.type;
				if ((type == V4L2_CTRL_TYPE_MENU) || (type == V4L2_CTRL_TYPE_INTEGER_MENU)) {
					const auto r = source->get_menu_items(id, val.items);
					if (UNLIKELY(r)) {
						LOGW("get_menu_items failed,%d", r);
					}
				}
			}
			values[id] = std::make_unique<osd_value_t>(val);
		}
	}
	// 自動露出中などで変更できない項目があるので一時的にenabledを落とす処理
	for (const auto id: AUTO_CTRLS) {
		update_constraints(id);
	}

	EXIT();
}

/**
 * @brief OSD表示中のキー入力処理
 *
 * @param event
 */
/*public*/
void OSD::on_key(const KeyEvent &event) {
	ENTER();

	if (event.key == ImGuiKey_Escape) {
		if ((event.action == KEY_ACTION_RELEASE)) {
			cancel();
		}
	} else {
		auto io = ImGui::GetIO();
		io.AddKeyEvent(event.key, (event.action == KEY_ACTION_PRESSED));
	}

	EXIT();
}

/**
 * @brief OSD表示の描画処理
 *
 */
/*public*/
void OSD::draw(ImFont *large_font) {
	ENTER();

	static bool visible = true;	// ダミー
	const static ImVec2 pivot(0.5f, 0.5f);
	const static ImGuiWindowFlags window_flags
		= ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoSavedSettings;

	float old_size;
	if (LIKELY(large_font)) {
		old_size = large_font->Scale;
		large_font->Scale /= 2;
		ImGui::PushFont(large_font);
	}

	const auto viewport = ImGui::GetMainViewport();
	const auto work_size = viewport->WorkSize;
	ImGui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, pivot);
	ImGui::SetNextWindowSize(ImVec2(work_size.x - OSD_FRAME_PADDING * 2, work_size.y - OSD_FRAME_PADDING * 2));
	ImGui::SetNextWindowBgAlpha(OSD_BK_ALPHA);
	ImGui::Begin("OSD", &visible, window_flags);
	{
		switch (page) {
		case PAGE_VERSION:
			draw_version();
			break;
		case PAGE_SETTINGS_1:
			draw_settings_1();
			break;
		case PAGE_ADJUST_1:
			draw_adjust_1();
			break;
		case PAGE_ADJUST_2:
			draw_adjust_2();
			break;
		case PAGE_ADJUST_3:
			draw_adjust_3();
			break;
		default:
			LOGD("unknown osd page,%d", page);
			break;
		}
	}
	ImGui::End();

	if (LIKELY(large_font)) {
		large_font->Scale = old_size;
		ImGui::PopFont();
	}

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief 保存して閉じる
 * 
 */
/*private*/
void OSD::save() {
	ENTER();

	const auto changed = camera_modified || app_settings.is_modified();
	// FIXME 変更されているかどうか
	// FIXME 未実装 保存処理
	app_settings.save();

	if (on_osd_close) {
		on_osd_close(changed);
	}
	values.clear();

	EXIT();
}

/**
 * @brief 保存せずに閉じる
 * 
 */
/*private*/
void OSD::cancel() {
	ENTER();

	camera_modified = false;
	if (on_osd_close) {
		on_osd_close(false);
	}
	values.clear();

	EXIT();
}

/**
 * @brief 前のーページへ移動
 * 
 */
/*private*/
void OSD::prev() {
	ENTER();

	if (page > 0) {
		page--;
	}

	EXIT();
}

/**
 * @brief 次のページへ移動
 * 
 */
/*private*/
void OSD::next() {
	ENTER();

	if (page < PAGE_NUM - 1) {
		page++;
	}

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief 機器情報画面
 */
/*private*/
void OSD::draw_version() {
	ENTER();

	const auto button_width = get_button_width();
	const auto size = ImVec2(button_width - 8/*ちょっとだけスペースが開くように*/, 0);

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		ImGui::LabelText("", "MODEL");
		ImGui::LabelText("", "VERSION");
		ImGui::LabelText("", "SERIAL");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::Spacing();
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{
		ImGui::LabelText("", "%s", MODEL);
		ImGui::LabelText("", "%s", VERSION);
		ImGui::LabelText("", "%s", get_serial().c_str());;
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::Spacing();
	}
	ImGui::EndGroup();

	if (ImGui::Button("CANCEL", size)) {
		cancel();
	}

	EXIT();
}

/**
 * @brief 設定画面１
 * 
 */
/*private*/
void OSD::draw_settings_1() {
	ENTER();

	const auto button_width = get_button_width();
	const auto size = ImVec2(button_width * 2, 0);

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		if (ImGui::Button(APP_LABEL_RESTORE_SETTINGS, size)) {
			const osd_value_t val = {
				.id = V4L2_CID_RESTORE_SETTINGS,
				.value {
					.id = V4L2_CID_RESTORE_SETTINGS,
				},
			};
			value_changed(val);
			if (on_osd_close) {
				on_osd_close(true);
			}
			values.clear();
		}
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::Spacing();
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::Spacing();
	}
	ImGui::EndGroup();

	draw_buttons_default();

	EXIT();
}

/**
 * @brief 調整画面1
 * 
 */
/*private*/
void OSD::draw_adjust_1() {
	ENTER();

	const auto button_width = get_button_width();

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		show_label(V4L2_CID_BRIGHTNESS, V4L2_LABEL_BRIGHTNESS);
		show_label(V4L2_CID_HUE, V4L2_LABEL_HUE);
		show_label(V4L2_CID_SATURATION, V4L2_LABEL_SATURATION);
		show_label(V4L2_CID_CONTRAST, V4L2_LABEL_CONTRAST);
		show_label(V4L2_CID_SHARPNESS, V4L2_LABEL_SHARPNESS);
		show_label(V4L2_CID_GAMMA, V4L2_LABEL_GAMMA);
		ImGui::Spacing();
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{
		ImGui::PushItemWidth(button_width * 2);
		show_value(V4L2_CID_BRIGHTNESS, V4L2_LABEL_BRIGHTNESS);
		show_value(V4L2_CID_HUE, V4L2_LABEL_HUE);
		show_value(V4L2_CID_SATURATION, V4L2_LABEL_SATURATION);
		show_value(V4L2_CID_CONTRAST, V4L2_LABEL_CONTRAST);
		show_value(V4L2_CID_SHARPNESS, V4L2_LABEL_SHARPNESS);
		show_value(V4L2_CID_GAMMA, V4L2_LABEL_GAMMA);
		ImGui::Spacing();
		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();

	draw_buttons_default();

	EXIT();
}

/**
 * @brief 調整画面2
 * 
 */
/*private*/
void OSD::draw_adjust_2() {
	ENTER();

	const auto button_width = get_button_width();

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		show_label(V4L2_CID_DENOISE, V4L2_LABEL_DENOISE);
		show_label(V4L2_CID_AUTOGAIN, V4L2_LABEL_AUTOGAIN);
		show_label(V4L2_CID_GAIN, V4L2_LABEL_GAIN);
		show_label(V4L2_CID_EXPOSURE_AUTO, V4L2_LABEL_EXPOSURE_AUTO);
		show_label(V4L2_CID_EXPOSURE, V4L2_LABEL_EXPOSURE);
		show_label(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, V4L2_LABEL_AUTO_N_PRESET_WHITE_BLANCE);
		ImGui::Spacing();
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{
		ImGui::PushItemWidth(button_width * 2);
		show_value(V4L2_CID_DENOISE, V4L2_LABEL_DENOISE);
		show_value(V4L2_CID_AUTOGAIN, V4L2_LABEL_AUTOGAIN);
		show_value(V4L2_CID_GAIN, V4L2_LABEL_GAIN);
		show_value(V4L2_CID_EXPOSURE_AUTO, V4L2_LABEL_EXPOSURE_AUTO);
		show_value(V4L2_CID_EXPOSURE, V4L2_LABEL_EXPOSURE);
		show_value(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, V4L2_LABEL_AUTO_N_PRESET_WHITE_BLANCE);
		ImGui::Spacing();
		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();

	draw_buttons_default();

	EXIT();
}

/**
 * @brief 調整画面3
 * 
 */
/*private*/
void OSD::draw_adjust_3() {
	ENTER();

	const auto button_width = get_button_width();

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		show_label(V4L2_CID_POWER_LINE_FREQUENCY, V4L2_LABEL_POWER_LINE_FREQUENCY);
		show_label(V4L2_CID_ZOOM_ABSOLUTE, V4L2_LABEL_ZOOM_ABSOLUTE);
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::Spacing();
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{
		ImGui::PushItemWidth(button_width * 2);
		show_value(V4L2_CID_POWER_LINE_FREQUENCY, V4L2_LABEL_POWER_LINE_FREQUENCY);
		show_value(V4L2_CID_ZOOM_ABSOLUTE, V4L2_LABEL_ZOOM_ABSOLUTE);
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::LabelText("", "");
		ImGui::Spacing();
		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();

	draw_buttons_default();

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief デフォルトのボタン幅を取得
 * 
 * @return float 
 */
/*private*/
float OSD::get_button_width() {
    const auto style = ImGui::GetStyle();
	const auto padding = style.WindowPadding.x;
	return (ImGui::GetWindowWidth() - padding * 2) / 4.0f;
}

/**
 * @brief デフォルトのボタンを描画する
 * 
 */
/*private*/
void OSD::draw_buttons_default() {
	ENTER();

    const auto style = ImGui::GetStyle();
	const auto padding = style.WindowPadding.x;
	const auto button_width	=(ImGui::GetWindowWidth() - padding * 2) / 4.0f;
	const auto size = ImVec2(button_width - 8/*ちょっとだけスペースが開くように*/, 0);

	if (ImGui::Button("CANCEL", size)) {
		cancel();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width + padding);
	if (ImGui::Button("SAVE", size)) {
		save();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width * 2 + padding);
	if (page > 0) {
		if (ImGui::Button("PREV", size)) {
			prev();
		}
	} else {
		ImGui::Spacing();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width * 3 + padding);
	if (page < PAGE_NUM - 1) {
		if (ImGui::Button("NEXT", size)) {
			next();
		}
	} else {
		ImGui::Spacing();
	}

	EXIT();
}

/**
 * 指定したidのカメラコントロールに対応している場合に指定したラベルを
 * ImGui::LabelTextとして表示する。
 * 対応していない場合は空のImGui::LabelTextを表示する。
 * @param id
 * @param label
*/
/*private*/
void OSD::show_label(const uint32_t &id, const char *label) {
	ENTER();

	const auto &val = values[id];
	if (val && val->supported) {
		ImGui::LabelText("", "%s", label);
	} else {
		ImGui::LabelText("", "");
	}

	EXIT();
}

/**
 * 指定したidのカメラコントロールに対応している場合に指定したラベルを持つ
 * ImGui::SliderIntを表示する。
 * 対応していない場合は空のImGui::LabelTextを表示する。
 * @param id
 * @param label
*/
/*private*/
void OSD::show_value(const uint32_t &id, const char *label) {

	ENTER();

	auto &_val = values[id];
	if (_val && _val->supported) {
		auto val = _val.get();
		if (val->enabled) {
			auto &v = val->value;
			const std::vector<std::string> &items = val->items;
			if ((v.type == V4L2_CTRL_TYPE_MENU) && !items.empty()) {
				std::string selected = items[v.current - v.min];
				bool changed = false;
				if (ImGui::BeginCombo(label, selected.c_str())) {
					for (int i = v.min; i <= v.max; i++) {
						const auto ix = i - v.min;
						const bool is_selected = (selected == items[ix]);
						if (ImGui::Selectable(items[ix].c_str(), is_selected)) {
							selected = items[ix];
							v.current = i;
							changed = true;
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				if (changed) {
					const auto r = value_changed(*val);
					if (LIKELY(!r)) {
						// 変更を正常に適用できたとき
						val->prev = v.current;
						val->modified = true;
						// 自動露出と露出やゲインなどのように相互依存する場合の制約を更新
						update_constraints(id);
					} else {
						// 適用できなかったときは前の値に戻す
						v.current = val->prev;
					}
				}
			} else {
				// V4L2_CTRL_TYPE_INTEGERまたはV4L2_CTRL_TYPE_BOOLEANのはず
				// FIXME stepを考慮してない
				if (ImGui::SliderInt(label, &v.current, v.min, v.max)) {
					if ((v.current != val->prev) && (v.current >= v.min) && (v.current <= v.max)) {
						// LOGD("on_changed:id=0x%08x,v=%d, prev=%d", id, v.current, val->prev);
						const auto r = value_changed(*val);
						if (LIKELY(!r)) {
							// 変更を正常に適用できたとき
							val->prev = v.current;
							val->modified = true;
							// 自動露出と露出やゲインなどのように相互依存する場合の制約を更新
							update_constraints(id);
						} else {
							// 適用できなかったときは前の値に戻す
							v.current = val->prev;
						}
					}
				}
			}
		} else {
			const std::string str = format("%d", val->value.current);
			const auto button_width = get_button_width();
			const auto center_x = ImGui::GetWindowSize().x * 0.5f;
			const auto text_width = ImGui::CalcTextSize(str.c_str()).x;
			ImGui::SetCursorPosX(center_x + button_width - text_width * 0.5f);
			ImGui::Text("%s", str.c_str());
		}
	} else {
		ImGui::LabelText("", "");
	}

	EXIT();
}

/**
 * 設定値が変更されたときの処理
*/
/*private*/
int OSD::value_changed(const osd_value_t &value) {
	ENTER();

	int result = -1;
	LOGD("id=0x%08x,v=%d", value.id, value.value.current);

	if (on_camera_settings_changed) {
		result = on_camera_settings_changed(value.value);
		if (!result) {
			camera_modified = true;
		}
	}

	RETURN(result, int);
}

/**
 * 自動露出とゲインや露出などのように相互依存する制約を更新する
 * @param value
*/
/*private*/
void OSD::update_constraints(const uint32_t &id) {
	ENTER();

	auto &val = values[id];
	if (val && val->supported) {
		// FIXME 未実装 ここの制約が正しいかどうか動作未確認
		switch (id) {
		case V4L2_CID_AUTOBRIGHTNESS:
			set_enabled(V4L2_CID_BRIGHTNESS, !val->value.current);
			break;
		case V4L2_CID_HUE_AUTO:
			set_enabled(V4L2_CID_HUE, !val->value.current);
			break;
		case V4L2_CID_AUTOGAIN:
			set_enabled(V4L2_CID_GAIN, !val->value.current);
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			set_enabled(V4L2_CID_GAIN, val->value.current == V4L2_EXPOSURE_MANUAL);
			set_enabled(V4L2_CID_EXPOSURE, val->value.current == V4L2_EXPOSURE_MANUAL);
			break;
		case V4L2_CID_AUTO_WHITE_BALANCE:
			set_enabled(V4L2_CID_DO_WHITE_BALANCE, !val->value.current);
			set_enabled(V4L2_CID_WHITE_BALANCE_TEMPERATURE, !val->value.current);
			break;
		}
	}

	EXIT();
}

/**
 * 指定したidのカメラコントロールに対応している場合に有効無効をセットする。
 * 対応していなければ何もしない
 * @param id
 * @param enabled
*/
/*private*/
void OSD::set_enabled(const uint32_t &id, const bool &enabled) {
	ENTER();

	auto &val = values[id];
	if (val && val->supported) {
		val->enabled = enabled;
	}

	EXIT();
}

}   // namespace serenegiant::app

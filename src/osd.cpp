#if 0    // set 0 if you need debug log, otherwise set 1
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

#include "const.h"
#include "osd.h"

namespace serenegiant::app {

// 輝度調整・拡大縮小率モード表示の背景アルファ
#define OSD_BK_ALPHA (0.5f)
#define OSD_FRAME_PADDING (30)

/**
 * @brief コンストラクタ
 *
 */
/*public*/
OSD::OSD()
:	page(PAGE_SETTINGS_1)
{
	ENTER();

	// FIXME 未実装

	EXIT();
}

/**
 * @brief デストラクタ
 *
 */
/*public*/
OSD::~OSD() {
	ENTER();

	// FIXME 未実装

	EXIT();
}

/**
 * @brief OSD表示中のキー入力処理
 *
 * @param event
 */
void OSD::on_key(const KeyEvent &event) {
	ENTER();

	LOGD("key=%d,scancode=%d/%s,action=%d,mods=%d",
		event.key, event.scancode, glfwGetKeyName(event.key, event.scancode),
		event.action, event.mods);
	// FIXME 未実装

	EXIT();
}

/**
 * @brief OSD表示の描画処理
 *
 */
/*public*/
void OSD::draw(ImFont *large_font) {
	ENTER();

	float old_size;
	if (LIKELY(large_font)) {
		old_size = large_font->Scale;
		large_font->Scale /= 2;
		ImGui::PushFont(large_font);
	}

	static bool visible = true;	// ダミー
	const static ImVec2 pivot(0.5f, 0.5f);
	const static ImGuiWindowFlags window_flags
		= ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav;
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

	// FIXME 未実装 保存処理
	if (on_osd_close) {
		on_osd_close();
	}

	EXIT();
}

/**
 * @brief 保存せずに閉じる
 * 
 */
/*private*/
void OSD::cancel() {
	ENTER();

	if (on_osd_close) {
		on_osd_close();
	}

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

/**
 * @brief 機器情報画面
 */
/*private*/
void OSD::draw_version() {
	ENTER();

    const auto style = ImGui::GetStyle();
	const auto padding = style.WindowPadding.x;
	const auto button_width = (ImGui::GetWindowWidth() - padding * 2) / 4.0f;
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
	if (ImGui::Button("RETURN", size)) {
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

    const auto style = ImGui::GetStyle();
	const auto padding = style.WindowPadding.x;
	const auto button_width = (ImGui::GetWindowWidth() - padding * 2) / 4.0f;
	const auto size = ImVec2(button_width - 8/*ちょっとだけスペースが開くように*/, 0);

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		ImGui::LabelText("", "RESTORE_SETTINGS");
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

	if (ImGui::Button("RETURN", size)) {
		cancel();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width + padding);
	if (ImGui::Button("SAVE", size)) {
		save();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width * 2 + padding);
	if (ImGui::Button("INFO", size)) {
		prev();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width * 3 + padding);
	if (ImGui::Button("NEXT", size)) {
		next();
	}

	EXIT();
}

/**
 * @brief 調整画面1
 * 
 */
/*private*/
void OSD::draw_adjust_1() {
	ENTER();

    const auto style = ImGui::GetStyle();
	const auto padding = style.WindowPadding.x;
	const auto button_width = (ImGui::GetWindowWidth() - padding * 2) / 4.0f;
	const auto size = ImVec2(button_width - 8/*ちょっとだけスペースが開くように*/, 0);

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		ImGui::LabelText("", "BRIGHTNESS");
		ImGui::LabelText("", "HUE");
		ImGui::LabelText("", "SATURATION");
		ImGui::LabelText("", "CONTRAST");
		ImGui::LabelText("", "SHARPNESS");
		ImGui::LabelText("", "GAMMA");
		ImGui::Spacing();
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{	// FIXME 未実装
		static float brightness = 10;
		static float hue = 20;
		static float saturation = 30;
		static float contrast = 40;
		static float sharpness = 50;
		static float gamma = 60;
		ImGui::PushItemWidth(button_width * 2);
		ImGui::SliderFloat("BRIGHTNESS", &brightness, 0.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("HUE", &hue, 0.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("SATURATION", &saturation, 0.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("CONTRAST", &contrast, 0.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("SHARPNESS", &sharpness, 0.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("GAMMA", &gamma, 0.0f, 100.0f, "%.0f");
		ImGui::Spacing();
		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();

	if (ImGui::Button("RETURN", size)) {
		cancel();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width + padding);
	if (ImGui::Button("SAVE", size)) {
		save();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width * 2 + padding);
	if (ImGui::Button("PREV", size)) {
		prev();
	}

	ImGui::SameLine(); ImGui::SetCursorPosX(button_width * 3 + padding);
	if (ImGui::Button("NEXT", size)) {
		next();
	}

	EXIT();
}

}   // namespace serenegiant::app

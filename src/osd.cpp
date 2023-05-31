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
:	page(PAGE_VERSION)
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

/**
 * @brief 機器情報画面
 */
void OSD::draw_version() {
	ENTER();

	// ラベルを描画(左半分)
	ImGui::BeginGroup();
	{
		ImGui::Text("MODEL");
		ImGui::Text("VERSION");
		ImGui::Text("SERIAL");
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::NewLine();
		if (ImGui::Button("RETURN")) {
			// FIXME OSDを閉じる
		}
	}
	ImGui::EndGroup(); ImGui::SameLine();
	// 値を描画(右半分)
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f);
	ImGui::BeginGroup();
	{
		ImGui::Text("%s", MODEL);
		ImGui::Text("%s", VERSION);
		ImGui::Text("%s", get_serial().c_str());;
	}
	ImGui::EndGroup();

	EXIT();
}

/**
 * @brief 設定画面１
 * 
 */
void OSD::draw_settings_1() {
	ENTER();

	// FIXME 未実装

	EXIT();
}

}   // namespace serenegiant::app

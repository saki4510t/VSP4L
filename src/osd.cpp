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

#include "osd.h"

namespace serenegiant::app {

// 輝度調整・拡大縮小率モード表示の背景アルファ
#define OSD_BK_ALPHA (0.5f)

/**
 * @brief コンストラクタ
 *
 */
/*public*/
OSD::OSD()
:	page(0)
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
void OSD::draw() {
	ENTER();

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
	const auto work_center = viewport->GetWorkCenter();
	const auto work_size = viewport->WorkSize;
	const auto rect_size = ImVec2(work_size.x / 4 * 3, work_size.y / 4 * 3);
	const auto rect = ImVec4(
		work_center.x - rect_size.x / 2, work_center.y - rect_size.y / 2,
		work_center.x + rect_size.x / 2, work_center.y + rect_size.y / 2);
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, pivot);
	ImGui::SetNextWindowSize(rect_size);
	ImGui::SetNextWindowBgAlpha(OSD_BK_ALPHA);
	ImGui::Begin("OSD", &visible, window_flags);
	{
		switch (page) {
		case PAGE_VERSION:
			draw_version(rect);
			break;
		case PAGE_SETTINGS_1:
			draw_settings_1(rect);
			break;
		default:
			LOGD("unknown osd page,%d", page);
			break;
		}
	}
	ImGui::End();

	EXIT();
}

void OSD::draw_version(const ImVec4 &rect) {
	ENTER();

	// FIXME 未実装

	EXIT();
}

void OSD::draw_settings_1(const ImVec4 &rect) {
	ENTER();

	// FIXME 未実装

	EXIT();
}

}   // namespace serenegiant::app

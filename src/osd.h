#ifndef OSD_H_
#define OSD_H_

#include "key_event.h"
#include "window.h"

namespace serenegiant::app {

enum {
	PAGE_VERSION = 0,
	PAGE_SETTINGS_1,
	PAGE_NUM,
} osd_page_t;

class OSD {
private:
	int page;

	void draw_version(const ImVec4 &rect);
	void draw_settings_1(const ImVec4 &rect);
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
	 * @brief OSD表示中のキー入力
	 *
	 * @param event
	 */
	void on_key(const KeyEvent &event);

	/**
	 * @brief OSD表示の描画処理
	 *
	 */
	void draw();
};

}   // namespace serenegiant::app

#endif  // OSD_H_

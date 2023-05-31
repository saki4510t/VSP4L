#ifndef OSD_H_
#define OSD_H_

#include "key_event.h"
#include "window.h"

namespace serenegiant::app {

enum {
	PAGE_VERSION = 0,
	PAGE_SETTINGS_1,
	PAGE_ADJUST_1,
	PAGE_NUM,
} osd_page_t;

class OSD {
private:
	int page;

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
	void draw(ImFont *large_font);
};

}   // namespace serenegiant::app

#endif  // OSD_H_

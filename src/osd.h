#ifndef OSD_H_
#define OSD_H_

#include "key_event.h"

namespace serenegiant::app {

class OSD {
private:
	int page;
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

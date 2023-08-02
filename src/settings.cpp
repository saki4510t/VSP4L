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

#include <stdio.h>
#include <string>
#include <cstring>

#include "utilbase.h"

#include "settings.h"

namespace serenegiant::app {

AppSettings::AppSettings()
:	modified(false)
{
	ENTER();
	EXIT();
}

AppSettings::~AppSettings() {
	ENTER();
	EXIT();
}

void AppSettings::clear() {
	ENTER();

	// FIXME 未実装

	EXIT();
}

//--------------------------------------------------------------------------------
CameraSettings::CameraSettings()
:	modified(false)
{
	ENTER();
	EXIT();
}

CameraSettings::~CameraSettings() {
	ENTER();
	EXIT();
}

/**
 * 自動露出モードかどうか
*/
bool CameraSettings::is_auto_exposure() const {
	ENTER();

	static const uint32_t id = V4L2_CID_EXPOSURE_AUTO;
	const auto itr = values.find(id);

	RETURN((itr != values.end()) && ((*itr).second == 0), bool);
}

/**
 * 自動ホワイトバランスかどうか
*/
bool CameraSettings::is_auto_white_blance() const {
	ENTER();

	static const uint32_t id = V4L2_CID_AUTO_WHITE_BALANCE;
	const auto itr = values.find(id);

	RETURN((itr != values.end()) && ((*itr).second == 0), bool);
}

/**
 * 自動輝度調整かどうか
*/
bool CameraSettings::is_auto_brightness() const {
	ENTER();

	static const uint32_t id = V4L2_CID_AUTOBRIGHTNESS;
	const auto itr = values.find(id);

	RETURN((itr != values.end()) && ((*itr).second != 0), bool);
}

/**
 * 自動色相調整かどうか
*/
bool CameraSettings::is_auto_hue() const {
	ENTER();

	static const uint32_t id = V4L2_CID_HUE_AUTO;
	const auto itr = values.find(id);

	RETURN((itr != values.end()) && ((*itr).second != 0), bool);
}

/**
 * 自動ゲイン調整かどうか
*/
bool CameraSettings::is_auto_gain() const {
	ENTER();

	static const uint32_t id = V4L2_CID_AUTOGAIN;
	const auto itr = values.find(id);

	RETURN((itr != values.end()) && ((*itr).second != 0), bool);
}

/**
 * 保持しているidと値をすべてクリアする
*/
void CameraSettings::clear() {
	ENTER();

	values.clear();
	modified = false;

	EXIT();
}

/**
 * 指定したidに対応する値をセットする
 * 値が追加または変更されたときはmodifiedフラグを立てる
 * @param id
 * @param val
*/
void CameraSettings::set_value(const uint32_t &id, const int32_t &val) {
	ENTER();

	const auto itr = values.find(id);
	modified |= (itr == values.end()) || (val != values[id]);

	values[id] = val;

	EXIT();
}

/**
 * 指定したidに対応する値を取得する
 * @param id
 * @param val
 * @return 0: 指定したidに対応する値が見つかった, それ以外: 指定したidに対応する値が見つからなかった
*/
int CameraSettings::get_value(const uint32_t &id, int32_t &val) {
	ENTER();

	int result = values.find(id) == values.end();
	if (!result) {
		val = values[id];
	}

	RETURN(result, int);
}

/**
 * 指定したidに対応する値を削除する
 * @param id
*/
void CameraSettings::remove(const uint32_t &id) {
	ENTER();

	for (auto itr = values.begin(), last = values.end(); itr != last;) {
		if ((*itr).first == id) {
			LOGD("remove,id=0x%08x", id);
			itr = values.erase(itr);
			modified = true;
		} else {
			itr++;
		}
	}

	EXIT();
}

//--------------------------------------------------------------------------------
/**
 * @brief アプリ設定を保存
 *
 * @param settings
 * @return int
 */
int save(AppSettings &settings) {
	ENTER();

	// FIXME 未実装
	settings.set_modified(false);

	RETURN(0, int);
}

/**
 * @brief アプリ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(AppSettings &settings) {
	ENTER();

	settings.clear();
	// FIXME 未実装
	settings.set_modified(false);

	RETURN(0, int);
}

/**
 * @brief カメラ設定を保存
 *
 * @param settings
 * @return int
 */
int save(CameraSettings &settings) {
	ENTER();

	// FIXME 未実装
	settings.set_modified(false);

	RETURN(0, int);
}

/**
 * @brief カメラ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(CameraSettings &settings) {
	ENTER();

	settings.clear();
	// FIXME 未実装
	settings.set_modified(false);

	RETURN(0, int);
}

}   // namespace serenegiant::app

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <unordered_map>

#include "const.h"

namespace serenegiant::app {

/**
 * @brief アプリ設定
 *
 */
class AppSettings {
private:
	bool modified;
protected:
public:
	AppSettings();
	~AppSettings();

	inline bool is_modified() const { return modified; };
	inline void set_modified(const bool &m) {
		modified = m;
	};
	void clear();
};

/**
 * @brief カメラ設定
 *
 */
class CameraSettings {
private:
	bool modified;
	std::unordered_map<uint32_t, int32_t> values;
protected:
public:
	CameraSettings();
	~CameraSettings();

	inline bool is_modified() const { return modified; };
	inline void set_modified(const bool &m) {
		modified = m;
	};

	/**
	 * 自動露出モードかどうか
	*/
	bool is_auto_exposure() const;
	/**
	 * 自動ホワイトバランスかどうか
	*/
	bool is_auto_white_blance() const;
	/**
	 * 自動輝度調整かどうか
	*/
	bool is_auto_brightness() const;
	/**
	 * 自動色相調整かどうか
	*/
	bool is_auto_hue() const;
	/**
	 * 自動ゲイン調整かどうか
	*/
	bool is_auto_gain() const;

	/**
	 * 保持しているidと値をすべてクリアする
	 * 値が追加または変更されたときはmodifiedフラグを立てる
	*/
	void clear();
	/**
	 * 指定したidに対応する値をセットする
	 * @param id
	 * @param val
	*/
	void set_value(const uint32_t &id, const int32_t &val);
	/**
	 * 指定したidに対応する値を取得する
	 * @param id
	 * @param val
	 * @return 0: 指定したidに対応する値が見つかった, それ以外: 指定したidに対応する値が見つからなかった
	*/
	int get_value(const uint32_t &id, int32_t &val);
	/**
	 * 指定したidに対応する値を削除する
	 * @param id
	*/
	void remove(const uint32_t &id);
};

//--------------------------------------------------------------------------------
/**
 * @brief アプリ設定を保存
 *
 * @param settings
 * @return int
 */
int save(AppSettings &settings);
/**
 * @brief アプリ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(AppSettings &settings);

/**
 * @brief カメラ設定を保存
 *
 * @param settings
 * @return int
 */
int save(CameraSettings &settings);
/**
 * @brief カメラ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(CameraSettings &settings);

}   // namespace serenegiant::app

#endif  // SETTINGS_H_

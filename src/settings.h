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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <string>
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

	/**
	 * @brief アプリ設定を保存
	 *
	 * @param path
	 * @return int
	 */
	int save(const std::string &path = "");
	/**
	 * @brief アプリ設定を読み込み
	 *
	 * @param path
	 * @return int
	 */
	int load(const std::string &path = "");

	inline bool is_modified() const { return modified; };
	inline void set_modified(const bool &m) {
		modified = m;
	};

	/**
	 * 設定が含まれないかどうかを取得
	 * @return true: 設定が含まれない、false: 設定が含まれている
	*/
	inline bool empty() const { return true; };

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

	/**
	 * @brief カメラ設定を保存
	 *
	 * @param path
	 * @return int
	 */
	int save(const std::string &path = "");
	/**
	 * @brief カメラ設定を読み込み
	 *
	 * @param path
	 * @return int
	 */
	int load(const std::string &path = "");

	inline bool is_modified() const { return modified; };
	inline void set_modified(const bool &m) {
		modified = m;
	};

	/**
	 * 設定が含まれないかどうかを取得
	 * @return true: 設定が含まれない、false: 設定が含まれている
	*/
	inline bool empty() const { return values.empty(); };

	/**
	 * 指定したidに対応する設定が含まれているかどうかを取得
	 * @param id
	*/
	inline bool contains(const uint32_t &id) const {
		return values.find(id) != values.end();
	}

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

}   // namespace serenegiant::app

#endif  // SETTINGS_H_

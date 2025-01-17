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

#define USE_FILE_SYSTEM (0)

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdio.h>
#include <string>

#if USE_FILE_SYSTEM
	#include <filesystem>
#else
	#include <sys/stat.h>
#endif

#include "utilbase.h"
// common
#include "charutils.h"
// app
#include "settings.h"

namespace serenegiant::app {

#define APP_SETTINGS_FILE "app.ini"
#define CAMERA_SETTINGS_FILE "camera.ini"

#if USE_FILE_SYSTEM
	namespace fs = std::filesystem;
#endif

static inline bool file_exists(const std::string &path) {
#if USE_FILE_SYSTEM
	// なぜかターゲット向けのyoctoビルドでコンパイルエラーになる、
	// ターゲット上でのセルフコンパイルならOK
	return fs::exists(fs::status(path.c_str()));
#else
	struct stat buffer;   
	return (stat (path.c_str(), &buffer) == 0); 
#endif
}

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

/**
 * @brief アプリ設定を保存
 *
 * @param path
 * @return int
 */
int AppSettings::save(const std::string &path) {
	ENTER();

	const std::string _path = !path.empty() ? path : APP_SETTINGS_FILE;
	std::ofstream out(_path.c_str());
	{
		// FIXME 未実装
	}	
	out.close();

	set_modified(false);

	RETURN(0, int);
}

/**
 * @brief アプリ設定を読み込み
 *
 * @param path
 * @return int
 */
int AppSettings::load(const std::string &path) {
	ENTER();

	const std::string _path = !path.empty() ? path : APP_SETTINGS_FILE;
	clear();
	if (file_exists(_path)) {
		std::ifstream in(_path.c_str());
		if (in.good()) {
			std::istream_iterator<std::string> it(in);
			std::istream_iterator<std::string> last;
			std::for_each(it, last, [this](std::string v) {
				const auto div = v.find("=");
				if (div != std::string::npos) {
					const auto key_str = v.substr(0, div);
					const auto val_str = v.substr(div + 1, v.size());
					// FIXME 未実装
					LOGD("kye=%s,val=%s", key_str.c_str(), val_str.c_str());
				}
			});
			in.close();
		}
	}
	set_modified(false);

	RETURN(0, int);
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
 * @brief カメラ設定を保存
 *
 * @param path
 * @return int
 */
int CameraSettings::save(const std::string &path) {
	ENTER();

	const std::string _path = !path.empty() ? path : CAMERA_SETTINGS_FILE;
	std::ofstream out(_path.c_str());
	{
		for (auto itr: values) {
			out << format("0x%08x=%d", itr.first, itr.second) << std::endl;
		}
	}
	out.close();

	set_modified(false);

	RETURN(0, int);
}

/**
 * @brief カメラ設定を読み込み
 *
 * @param path
 * @return int
 */
int CameraSettings::load(const std::string &path) {
	ENTER();

	const std::string _path = !path.empty() ? path : CAMERA_SETTINGS_FILE;
	clear();
	if (file_exists(_path)) {
		std::ifstream in(_path.c_str());
		if (in.good()) {
			std::istream_iterator<std::string> it(in);
			std::istream_iterator<std::string> last;
			std::for_each(it, last, [this](std::string v) {
				const auto div = v.find("=");
				if (div != std::string::npos) {
					const auto id_str = v.substr(0, div);
					const auto val_str = v.substr(div + 1, v.size());
					LOGD("%s=%s", id_str.c_str(), val_str.c_str());
					try {
						const auto id = (uint32_t)(std::stol(id_str, nullptr, 0) & 0xffffff);
						const auto val = std::stoi(val_str);
						set_value(id, val);
					} catch (...) {
						LOGD("Failed to convert,%s", v.c_str());
					}
				}
			});
			in.close();
		}
	}
	// フレームレート指定は受け付けない
	remove(V4L2_CID_FRAMERATE);
	set_modified(false);

	RETURN(0, int);
}

/**
 * 自動露出モードかどうか
*/
bool CameraSettings::is_auto_exposure() const {
	ENTER();

	static const uint32_t id = V4L2_CID_EXPOSURE_AUTO;
	const auto itr = values.find(id);

	RETURN((itr != values.end()) && ((*itr).second != 1), bool);
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

	modified |= !contains(id) || (val != values[id]);

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

	int result = -1;
	if (contains(id)) {
		val = values[id];
		result = 0;
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

}   // namespace serenegiant::app

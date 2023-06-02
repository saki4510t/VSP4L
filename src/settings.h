#ifndef SETTINGS_H_
#define SETTINGS_H_

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
};

/**
 * @brief カメラ設定
 *
 */
class CameraSettings {
private:
	bool modified;
protected:
public:
	CameraSettings();
	~CameraSettings();

	inline bool is_modified() const { return modified; };
	inline void set_modified(const bool &m) {
		modified = m;
	};
};

//--------------------------------------------------------------------------------
/**
 * @brief アプリ設定を保存
 *
 * @param settings
 * @return int
 */
int save(const AppSettings &settings);
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
int save(const CameraSettings &settings);
/**
 * @brief カメラ設定を読み込み
 *
 * @param settings
 * @return int
 */
int load(CameraSettings &settings);

}   // namespace serenegiant::app

#endif  // SETTINGS_H_

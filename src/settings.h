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
protected:
public:
    AppSettings();
    ~AppSettings();
};

/**
 * @brief カメラ設定
 * 
 */
class CameraSettings {
private:
protected:
public:
    CameraSettings();
    ~CameraSettings();
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

/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef USBWEBCAMERAPROJ_CHARUTILS_H
#define USBWEBCAMERAPROJ_CHARUTILS_H

#include <string>
#include <cstring>
#include <vector>

namespace serenegiant {

/**
 * 指定した文字列にに指定した文字列が含まれているかどうかをチェック
 * @param s チェックされてる側の文字列
 * @param v 含まれているかチェックする文字列
 * @return
 */
bool contains(const std::string &s, const std::string &v);

/**
 * 指定した文字列が指定した文字列で開始しているかどうかをチェック
 * @param s
 * @param prefix
 * @return true: 指定した文字列が指定した文字列で開始している、false: 開始していない
 */
bool start_width(const std::string &s, const std::string &prefix);

/**
 * 指定した文字列が指定した文字列で終わっているかどうかをチェック
 * @param s
 * @param fuffix
 * @return true: 指定した文字列が指定した文字列で終了している、false: 終了していない
 */
bool end_with(const std::string &s, const std::string &suffix);

/**
 * 指定したC文字列が指定したC文字列で終わっているかどうかをテスト
 * @param text
 * @param suffix
 * @return
 */
bool end_width(const char *text, const char *suffix);

/**
 * c++でstd::stringに対してprintf風の書式設定で文字列を生成するためのラッパー関数
 * @tparam Args
 * @param fmt
 * @param args
 * @return
 */
template <typename ... Args>
std::string format(const std::string& fmt, Args ... args) {
    const size_t len = std::snprintf( nullptr, 0, fmt.c_str(), args ... );
    std::vector<char> buf(len + 1);
    std::snprintf(&buf[0], len + 1, fmt.c_str(), args ... );
    return std::string(&buf[0], &buf[0] + len);
}

}	// end of namespace serenegiant

#endif //USBWEBCAMERAPROJ_CHARUTILS_H

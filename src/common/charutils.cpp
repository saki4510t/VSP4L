/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1	// set 1 if you don't need debug message
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// ignore LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG		// depends on definition in Android.mk and Application.mk
#endif

#include <cstdlib>
#include <cstdio>
#include <algorithm> // std::equal
#include <iterator>  // std::rbegin, std::rend
#include <charconv>

#include "utilbase.h"
#include "charutils.h"

namespace serenegiant {

/**
 * 指定した文字列にに指定した文字列が含まれているかどうかをチェック
 * @param s チェックされてる側の文字列
 * @param v 含まれているかチェックする文字列
 * @return
 */
bool contains(const std::string &s, const std::string &v) {
   return s.find(v) != std::string::npos;
}

/**
 * 指定した文字列が指定した文字列で開始しているかどうかをチェック
 * @param s
 * @param t
 * @return true: 指定した文字列が指定した文字列で開始している、false: 開始していない
 */
bool start_width(const std::string &s, const std::string &prefix) {
	return (s.size() >= prefix.size())
		&& std::equal(std::begin(prefix), std::end(prefix), std::begin(s));
}

/**
 * 指定した文字列が指定した文字列で終わっているかどうかをチェック
 * @param s
 * @param fuffix
 * @return true: 指定した文字列が指定した文字列で終了している、false: 終了していない
 */
bool end_with(const std::string &s, const std::string &suffix) {
   return (s.size() >= suffix.size())
	&& std::equal(std::rbegin(suffix), std::rend(suffix), std::rbegin(s));
}

/**
 * 指定したC文字列が指定したC文字列で終わっているかどうかをテスト
 * @param text
 * @param suffix
 * @return
 */
bool end_width(const char *text, const char *suffix) {
	bool result = false;
	const size_t text_len = text ? strlen(text) : 0;
	const size_t target_len = suffix ? strlen(suffix) : 0;
	if (text_len >= target_len) {
		result = true;
		for (int i = 1; i <= target_len; i++) {
			if (text[text_len - i] != suffix[target_len - i]) {
				result = false;
				break;
			}
		}
	}
	return result;
}

/**
 * @brief デフォルト値付きで文字列を整数へ変換する
 * 
 * @param str 変換する文字列, const char*, string, string_viewのいずれか
 * @param default_value 変換できないときのデフォルト値
 * @param radix 基数
 * @return int 
 */
int to_int(const std::string_view str, const int &default_value, const int &radix) {

	int result = default_value;
	const auto last = str.data() + str.size();
    const auto [ptr, ec] = std::from_chars(std::begin(str), std::end(str), result, radix);
	if (ec != std::errc{} || (ptr != last)) {
		result = default_value;
    }

	return result;
}

} // end of namespace serenegiant

/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <cstring> // memcpy

#include "utilbase.h"
// core
#include "core/frame_base.h"

namespace serenegiant::core {

/**
 * サイズ指定付きコンストラクタ
 * @param bytes
 */
BaseFrame::BaseFrame(const uint32_t &bytes)
:	_actual_bytes(bytes),
	_presentation_time_us(0), _received_sys_time_us(0),
	_sequence(0), _flags(0), _option(0),
	_frame(bytes)
{
	ENTER();
	EXIT();
}

/**
 * コピーコンストラクタ
 * @param src
 */
BaseFrame::BaseFrame(const BaseFrame &other)
:	_actual_bytes(other._actual_bytes),
	_presentation_time_us(other._presentation_time_us),
	_received_sys_time_us(other._received_sys_time_us),
	_sequence(other._sequence),
	_flags(other._flags), _option(other._option),
	_frame(_actual_bytes)
{
	ENTER();

	if (_actual_bytes) {
		memcpy(&_frame[0], &other._frame[0], _actual_bytes);
	}

	EXIT();
}

/**
 * デストラクタ
 */
BaseFrame::~BaseFrame() noexcept {
	ENTER();
	EXIT();
}

// 代入演算子(ディープコピー)
// デフォルトの処理より自前で定義した方が早いみたい
BaseFrame &BaseFrame::operator=(const BaseFrame &other) {
	ENTER();

	if (this != &other) {
//		_actual_bytes = other._actual_bytes;	// これはここでコピーしちゃダメ、後でset_sizeした時に設定される
		_presentation_time_us = other._presentation_time_us;
		_received_sys_time_us = other._received_sys_time_us;
		_sequence = other._sequence;
		_flags = other._flags;
		_option = other._option;
		const auto bytes = set_size(other._actual_bytes);
		memcpy(&_frame[0], &other[0], bytes);
	}

	RET(*this);
}

// 代入演算子(ディープコピー)
// デフォルトの処理より自前で定義した方が早いみたい
BaseFrame &BaseFrame::operator=(const IFrame &other) {
	ENTER();

	auto _other = dynamic_cast<const BaseFrame *>(&other);
	if (_other && this != _other) {
//		_actual_bytes = other._actual_bytes;	// これはここでコピーしちゃダメ、後でset_sizeした時に設定される
		_presentation_time_us = _other->_presentation_time_us;
		_received_sys_time_us = _other->_received_sys_time_us;
		_sequence = _other->_sequence;
		_flags = _other->_flags;
		_option = _other->_option;
		const auto bytes = set_size(_other->_actual_bytes);
		memcpy(&_frame[0], &other[0], bytes);
	}

	RET(*this);
}

/**
 * 配列添え字演算子
 * @param ix
 * @return
 */
uint8_t &BaseFrame::operator[](uint32_t ix) {
	return _frame[ix];
}

/**
 * 配列添え字演算子
 * @param ix
 * @return
 */
/*public*/
const uint8_t &BaseFrame::operator[](uint32_t ix) const {
	return _frame[ix];
}

/**
 * 機器から受け取ったPTSを取得
 * @return
 */
/*public*/
nsecs_t BaseFrame::presentation_time_us() const {
	return _presentation_time_us;
}

/**
 * フレームデータ受信時のシステム時刻[マイクロ秒]を取得する
 * @return
 */
nsecs_t BaseFrame::received_sys_time_us() const {
	return _received_sys_time_us;
}

/**
 * フレームシーケンス番号を取得
 * @return
 */
uint32_t BaseFrame::sequence() const {
	return _sequence;
};

/**
 * フレームのフラグを取得
 * @return
 */
uint32_t BaseFrame::flags() const {
	return _flags;
}

/**
 * フレームのオプションフラグを取得
 * @return
 */
uint32_t BaseFrame::option() const {
	return _option;
}

/**
 * 保持しているデータサイズを取得
 * #actual_bytes <= #raw_bytes <= #size
 * @return
 */
/*public*/
size_t BaseFrame::actual_bytes() const {
	return raw_bytes();
}

/**
 * バッファサイズを取得(バッファを拡張せずに保持できる最大サイズ)
 * #actual_bytes <= #raw_bytes <= #size
 * @return
 */
/*public*/
size_t BaseFrame::size() const {
	return _frame.size();
}

/**
 * フレームバッファーの先頭ポインタを取得
 * @return
 */
/*public*/
uint8_t *BaseFrame::frame() {
	return raw_frame(0);
}

/**
 * フレームバッファーの先頭ポインタを取得
 * @return
 */
/*public*/
const uint8_t *BaseFrame::frame() const {
	return raw_frame(0);
}

/**
 * 必要に応じてバッファサイズを変更してフレームデータをコピーする
 * @param data
 * @param bytes
 */
/*public*/
void BaseFrame::setFrame(const uint8_t *data, const size_t &bytes) {
	set_size(bytes);
	memcpy(&_frame[0], data, bytes);
}

/**
 * 現在のバッファの最後に新しいデータを追加する
 * バッファサイズが足らないときはバッファサイズを拡張する
 * @param data
 * @param bytes
 * @return 0: 追加に成功した それ以外: 追加できなかった
 */
int BaseFrame::append(const uint8_t *data, const size_t &bytes) {
	ENTER();

	if (UNLIKELY(!data || !bytes)) {
		RETURN(0, int);
	}

	int result = 0;
	const auto current_bytes = actual_bytes();
	if (UNLIKELY(current_bytes + bytes > size())) {
		// バッファサイズが足りなくなったとき
		if (current_bytes + bytes <= _frame.capacity()) {
			// メモリーを再確保せずにサイズ変更できる場合
			_frame.resize(current_bytes + bytes);
		} else {
			// メモリーの再確保が必要になる場合は現在のバッファ内容をワークへコピーして退避/復帰させる
			std::vector<uint8_t> work(current_bytes);
			memcpy(&work[0], &_frame[0], current_bytes);
			// バッファをリサイズ, XXX set_size呼び出しでactual_bytesが新しいサイズになるので注意！
			const auto new_size = set_size(current_bytes + bytes);
			if (new_size >= current_bytes + bytes) {
				// リサイズ成功したときは退避していたデータを書き戻す
				memcpy(&_frame[0], &work[0], current_bytes);
			} else {
				// リサイズ失敗したとき
				result = -1;
			}
		}
	}
	if (LIKELY(!result)) {
		// バッファの最後に新しいデータを追加
		memcpy(&_frame[current_bytes], data, bytes);
		_actual_bytes = current_bytes + bytes;
	}

	RETURN(result, int);
}

/**
 * バッファサイズを変更
 * サイズが大きくなる時以外は実際のバッファのサイズ変更はしない
 * width, height, pixelBytes, stepは変更しない
 * @param bytes バッファサイズ
 * @return actual_bytes
 */
/*public*/
size_t BaseFrame::set_size(const size_t &bytes) {
	_actual_bytes = bytes;
	if (UNLIKELY(bytes > size())) {		// サイズが大きくなる時以外は実際のバッファのサイズ変更はしない
		MARK("resize:%" FMT_SIZE_T "=>%d", _frame.size(), bytes);
		try {
			_frame.resize(bytes, 0);
		} catch (std::exception & e) {
			LOGE("failed to resize frame buffer");
		}
		_actual_bytes = _frame.size();
	}
	RETURN(raw_bytes(), int);
}

/*public*/
void BaseFrame::recycle() {
	_actual_bytes = 0;
	_frame.clear();
	_frame.shrink_to_fit();
}

/*public*/
void BaseFrame::clear() {
	_actual_bytes = 0;
}

}	// namespace serenegiant::usb

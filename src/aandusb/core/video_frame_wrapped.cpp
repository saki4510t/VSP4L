/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#define LOG_TAG "WrappedVideoFrame"

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
//	#define DEBUG_GL_CHECK			// GL関数のデバッグメッセージを表示する時
#endif

#include <cstring>

#include "utilbase.h"
// core
#include "core/video_frame_wrapped.h"
// usb
#include "usb/aandusb.h"

namespace serenegiant::core {

/**
 * コンストラクタ
 * @param buffer
 * @param buffer_bytes
 * @param width
 * @param height
 * @param frame_type
 */
WrappedVideoFrame::WrappedVideoFrame(
	uint8_t *buffer, const size_t buffer_bytes,
	const uint32_t &width, const uint32_t &height,
	const raw_frame_t &frame_type)
:	buffer(buffer), buffer_bytes(buffer_bytes),
	// IFrame実装用
	_actual_bytes(0),
	_presentation_time_us(0), _received_sys_time_us(0),
	_sequence(0), _flags(0), _option(0),
	// IVideoFrame実装用
	_frame_type(frame_type),
	_width(width), _height(height),
	_step(get_pixel_bytes(frame_type).frame_bytes(width, 1)),
	_pixelBytes(get_pixel_bytes(frame_type)),
	_pts(0), _stc(0), _sof(0),
	_packet_id(0),
	_frame_num_in_payload(1)
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
WrappedVideoFrame::~WrappedVideoFrame() noexcept {
	ENTER();
	EXIT();
}

/**
 * 代入演算子(ディープコピー)
 * @param src
 * @return
 */
WrappedVideoFrame &WrappedVideoFrame::operator=(const WrappedVideoFrame &src) {
	ENTER();
	
	auto _src = dynamic_cast<const WrappedVideoFrame *>(&src);
	if (_src && (this != _src)) {
//		_actual_bytes = other._actual_bytes;	// これはここでコピーしちゃダメ、後でset_sizeした時に設定される
		_presentation_time_us = _src->_presentation_time_us;
		_received_sys_time_us = _src->_received_sys_time_us;
		_sequence = _src->_sequence;
		_flags = _src->_flags;
		_option = _src->_option;
		const size_t bytes = resize(_src->_actual_bytes);
		memcpy(frame(), _src->frame(), bytes);
		_frame_type = _src->_frame_type;
		_width = _src->_width;
		_height = _src->_height;
		_step = _src->_step;
		_pixelBytes = _src->_pixelBytes;
		_pts = _src->_pts;
		_stc = _src->_stc;
		_sof = _src->_sof;
		_packet_id = _src->_packet_id;
		_frame_num_in_payload = _src->_frame_num_in_payload;
	}
	
	RET(*this);
}

/**
 * ディープコピー
 * @param dst
 * @return
 */
int WrappedVideoFrame::copy_to(IVideoFrame &dst) const {
	ENTER();

	int result;
	auto _dst = dynamic_cast<WrappedVideoFrame *>(&dst);
	if (_dst) {
		*_dst = *this;
		result = USB_SUCCESS;
	} else {
		result = dst.resize(*this, _frame_type);
		if (!result) {
			memcpy(dst.frame(), frame(), dst.actual_bytes());
		}
		dst.set_attribute(*this);
	}
	
	RETURN(result, int);
}

/**
 * 映像データをVideoImage_t形式で取得する
 * @param image
 * @return
 */
int WrappedVideoFrame::get_image(VideoImage_t &image) {
	ENTER();
	// ここのassignはvideo_image.h/video_image.cpp
	RETURN(core::assign(image, frame_type(), width(), height(), frame(), actual_bytes()), int);
};

/**
 * @brief 別の外部メモリーへ割り当て直す
 * 
 * @param buf 
 * @param size 
 * @param width
 * @param height
 * @param frame_type
 * 
 */
void WrappedVideoFrame::assign(uint8_t *buf, const size_t &size,
	const uint32_t &width, const uint32_t &height,
	const raw_frame_t &frame_type) {

	ENTER();

	buffer = buf;
	buffer_bytes = size;

	if (LIKELY(width && height && frame_type)) {
		_actual_bytes = size;
		_width = width;
		_height = height;
		_frame_type = frame_type;
		_step = get_pixel_bytes(frame_type).frame_bytes(width, 1);
		_pixelBytes = get_pixel_bytes(frame_type);
	} else {
		_actual_bytes = 0;
	}

	EXIT();
}

//--------------------------------------------------------------------------------
// IFrameの純粋仮想関数の実装
/**
 * 配列添え字演算子
 * プレフィックス等を考慮した値を返す
 * @param ix
 * @return
 */
uint8_t &WrappedVideoFrame::operator[](uint32_t ix) {
	return buffer[ix];
}

/**
 * 配列添え字演算子
 * プレフィックス等を考慮した値を返す
 * @param ix
 * @return
 */
const uint8_t &WrappedVideoFrame::operator[](uint32_t ix) const {
	const static uint8_t zero = 0;
	return buffer ? buffer[ix] : zero;
}

/**
 * 保持しているデータサイズを取得
 * プレフィックス等を考慮した値を返す
 * (バッファの実際のサイズではない)
 * #actual_bytes <= #raw_bytes <= #size
 * @return
 */
size_t WrappedVideoFrame::actual_bytes() const {
	return _actual_bytes;
}

/**
 * バッファサイズを取得(バッファを拡張せずに保持できる最大サイズ)
 * #actual_bytes <= #raw_bytes <= #size
 * @return
 */
size_t WrappedVideoFrame::size() const {
	return buffer_bytes;
}

/**
 * フレームバッファーの先頭ポインタを取得
 * プレフィックス等を考慮した値を返す
 * @return
 */
uint8_t *WrappedVideoFrame::frame() {
	return buffer;
}

/**
 * フレームバッファーの先頭ポインタを取得
 * プレフィックス等を考慮した値を返す
 * @return
 */
const uint8_t *WrappedVideoFrame::frame() const {
	return buffer;
}

/**
 * フレームデータのPTS[マイクロ秒]をを取得
 * @return
 */
nsecs_t WrappedVideoFrame::presentation_time_us() const {
	return _presentation_time_us;
}

/**
 * フレームデータ受信時のシステム時刻[マイクロ秒]を取得する
 * @return
 */
nsecs_t  WrappedVideoFrame::received_sys_time_us() const {
	return _received_sys_time_us;
};

/**
 * フレームシーケンス番号を取得
 * @return
 */
uint32_t WrappedVideoFrame::sequence() const {
	return _sequence;
}

/**
 * フレームのフラグを取得
 * @return
 */
uint32_t WrappedVideoFrame::flags() const {
	return _flags;
}

/**
 * フレームのオプションフラグを取得
 * @return
 */
uint32_t WrappedVideoFrame::option() const {
	return _option;
}

/**
 * 保持しているデータサイズをクリアする, 実際のバッファ自体はそのまま
 */
void WrappedVideoFrame::clear() {
	_actual_bytes = 0;
}

//--------------------------------------------------------------------------------
// IVideoFrameの純粋仮想関数の実装
/**
 * フレームタイプ(映像データの種類)を取得
 * @return
 */
raw_frame_t WrappedVideoFrame::frame_type() const {
	return _frame_type;
}

/**
 * 映像データの横幅(ピクセル数)を取得
 * @return
 */
uint32_t WrappedVideoFrame::width() const {
	return _width;
}

/**
 * 映像データの高さ(ピクセル数)を取得
 * @return
 */
uint32_t WrappedVideoFrame::height() const {
	return _height;
}

/**
 * 映像データ1行(横幅x1)分のバイト数を取得
 * @return
 */
uint32_t WrappedVideoFrame::step() const {
	return _step;
}

/**
 * 1ピクセルあたりのバイト数を取得
 * @return
 */
const PixelBytes &WrappedVideoFrame::pixel_bytes() const {
	return _pixelBytes;
}

/**
 * メージセンサーから映像を取得開始した時のクロック値
 * @return
 */
uint32_t WrappedVideoFrame::pts() const {
	return _pts;
}

/**
 * 取得したイメージをUSBバスにセットし始めたときのクロック値
 * @return
 */
uint32_t WrappedVideoFrame::stc() const {
	return _stc;
}

/**
 * stcを取得したときの１KHｚ SOFカウンタの値
 * @return
 */
uint32_t WrappedVideoFrame::sof() const {
	return _sof;
}

/**
 * パケットIDを取得
 * @return
 */
size_t WrappedVideoFrame::packet_id() const {
	return _packet_id;
}

/**
 * ペイロード中のマイクロフレームの数を取得
 * @return
 */
int WrappedVideoFrame::frame_num_in_payload() const {
	return _frame_num_in_payload;
}

/**
 * 映像サイズとフレームタイプを設定
 * @param new_width
 * @param new_height
 * @param new_frame_type
 */
void WrappedVideoFrame::set_format(const uint32_t &width, const uint32_t &height,
	const raw_frame_t &frame_type) {

	resize(width, height, get_pixel_bytes(frame_type));
	_frame_type = frame_type;
}

/**
 * フレームタイプを設定
 * @param new_format
 */
void WrappedVideoFrame::set_format(const raw_frame_t &frame_type) {
	resize(width(), height(), get_pixel_bytes(frame_type));
	_frame_type = frame_type;
}

/**
 * バッファサイズを変更
 * 外部メモリーなので実際のリサイズはできない、actual_bytesを変更するだけ
 * width, height, pixelBytes, stepは変更しない
 * @param bytes バッファサイズ
 * @return actual_bytes この#resizeだけactual_bytesを返すので注意, 他は成功したらUSB_SUCCESS(0), 失敗したらエラーコード
 */
size_t WrappedVideoFrame::resize(const size_t &bytes) {
	ENTER();

	if (bytes <= size()) {
		_actual_bytes = bytes;
	}

	RETURN(size(), int);
}

/**
 * バッファサイズを変更
 * @param new__width
 * @param new__height
 * @param new__pixel_bytes 1ピクセルあたりのバイト数, 0なら実際のバッファサイズの変更はせずにwidth, height等を更新するだけ
 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
 */
int WrappedVideoFrame::resize(const uint32_t &new_width, const uint32_t &new_height,
	const PixelBytes &new_pixel_bytes) {
	
	ENTER();

	int result = USB_SUCCESS;
	const size_t new_frame_bytes = new_pixel_bytes.frame_bytes(new_width, new_height);
	if ((_width != new_width) || (_height != new_height)
		|| (new_frame_bytes > size())) {

		MARK("resize:%" FMT_SIZE_T "=>%" FMT_SIZE_T, size(), new_frame_bytes);
		if (new_frame_bytes <= size()) {
			_pixelBytes = new_pixel_bytes;
			_actual_bytes = new_frame_bytes;
			_width = new_width;
			_height = new_height;
			_pixelBytes = new_pixel_bytes;
			// FIXME I420とかだと正しくない
			_step = new_pixel_bytes.frame_bytes(new_width, 1);
		} else {
			result = USB_ERROR_NO_MEM;
		}
	}
	
	RETURN(result, int);
}

/**
 * バッファサイズを変更
 * @param new_width
 * @param new_height
 * @param new_frame_type
 * @return
 */
int WrappedVideoFrame::resize(const uint32_t &new_width, const uint32_t &new_height,
	const raw_frame_t &new_frame_type) {
	
	const int result = WrappedVideoFrame::resize(new_width, new_height, get_pixel_bytes(new_frame_type));
	if (!result) {
		_frame_type = new_frame_type;
	}
	return result;
}

/**
 * バッファサイズを変更
 * @param other widthと高さはこのVideoFrameインスタンスのものを使う
 * @param new__frame_type フレームの種類(raw_frame_t)
 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
 */
int WrappedVideoFrame::resize(const IVideoFrame &other, const raw_frame_t &new_frame_type) {
	const int result = WrappedVideoFrame::resize(other.width(), other.height(), get_pixel_bytes(new_frame_type));
	if (!result) {
		_frame_type = new_frame_type;
		set_attribute(other);
	}
	return result;
}

/**
 * ヘッダー情報を設定
 * @param pts
 * @param stc
 * @param sof
 * @return
 */
int WrappedVideoFrame::set_header_info(const uint32_t &pts, const uint32_t &stc, const uint32_t &sof) {
	ENTER();

   	_pts = pts;
   	_stc = stc;
   	_sof = sof;

	RETURN(USB_SUCCESS, int);
}

/**
 * _presentationtime_us, _received_sys_time_us, sequence, _flags等の
 * 映像データ以外をコピーする
 * @param src
 */
void WrappedVideoFrame::set_attribute(const IVideoFrame &src) {
	_presentation_time_us = src.presentation_time_us();
	_received_sys_time_us = src.received_sys_time_us();
	_sequence = src.sequence();
	_flags = src.flags();
	_option = src.option();
	_stc = src.stc();
	_sof = src.sof();
	_packet_id = src.packet_id();
	_frame_num_in_payload = src.frame_num_in_payload();
}

}	// namespace serenegiant::core

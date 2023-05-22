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

#include <cstring>

#include "utilbase.h"
// core
#include "core/video_frame_base.h"
// usb
#include "usb/aandusb.h"

namespace serenegiant::core {

/**
 * raw_frame_tから1ピクセル当たりのバイト数を取得する
 * MJPEG/H264/VP8は0を返すので注意
 */
/*@NonNull*/
PixelBytes get_pixel_bytes(const raw_frame_t &frame_type) {
	auto pixel_bytes = PixelBytes(1);
	switch (frame_type) {
	case RAW_FRAME_UNCOMPRESSED_YUYV:
	case RAW_FRAME_UNCOMPRESSED_UYVY:
		pixel_bytes = 2;
		break;
	case RAW_FRAME_UNCOMPRESSED_GRAY8:
	case RAW_FRAME_UNCOMPRESSED_BY8:
		break;
	case RAW_FRAME_UNCOMPRESSED_NV21:	// YVU420 SemiPlanar(y->vu)
	case RAW_FRAME_UNCOMPRESSED_YV12:	// YVU420 Planar(y->v->u)
	case RAW_FRAME_UNCOMPRESSED_I420:	// YVU420 Planar(y->u->v)
	case RAW_FRAME_UNCOMPRESSED_Y16:
		pixel_bytes = PixelBytes(3, 2);
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBP:
		pixel_bytes = 3;
		break;
	case RAW_FRAME_UNCOMPRESSED_M420:
	case RAW_FRAME_UNCOMPRESSED_NV12:	// YUV420 SemiPlanar NV21とU/Vの並びが逆(y->uv)
	case RAW_FRAME_UNCOMPRESSED_RGB565:	// インターリーブ
		pixel_bytes = 2;
		break;
	case RAW_FRAME_UNCOMPRESSED_RGB:
	case RAW_FRAME_UNCOMPRESSED_BGR:
		pixel_bytes = 3;
		break;
	case RAW_FRAME_UNCOMPRESSED_RGBX:
	case RAW_FRAME_UNCOMPRESSED_XRGB:
	case RAW_FRAME_UNCOMPRESSED_BGRX:
	case RAW_FRAME_UNCOMPRESSED_XBGR:
		pixel_bytes = 4;
		break;
	case RAW_FRAME_MJPEG:		// MJPEG
	case RAW_FRAME_MPEG2TS:
	case RAW_FRAME_H264:		// H264
	case RAW_FRAME_VP8:			// VP8
	case RAW_FRAME_FRAME_H264:	// H264
	case RAW_FRAME_FRAME_VP8:	// VP8
	case RAW_FRAME_DV:			// DV
	case RAW_FRAME_H265:		// H265
		// これはサイズ不明...4を返しといたほうが安全かも
		break;
	case RAW_FRAME_UNCOMPRESSED_444p:	// YVU444 Planar(y->u->v)
	case RAW_FRAME_UNCOMPRESSED_444sp:	// YVU444 semi Planar(y->uv)
		pixel_bytes = 4;
		break;
	case RAW_FRAME_UNCOMPRESSED_YCbCr:	// YUV Planer(YUV4:2:2)
	case RAW_FRAME_UNCOMPRESSED_422p:	// YVU422 Planar(y->u->v)
	case RAW_FRAME_UNCOMPRESSED_422sp:	// YVU422 semi Planar(y->uv)
	case RAW_FRAME_UNCOMPRESSED_440p:	// YVU440 Planar(y->u->v)
	case RAW_FRAME_UNCOMPRESSED_440sp:	// YVU440 semi Planar(y->uv)
		pixel_bytes = 2;
		break;
	case RAW_FRAME_UNCOMPRESSED_411p:	// YVU411 Planar(y->u->v)
	case RAW_FRAME_UNCOMPRESSED_411sp:	// YVU411 semi Planar(y->uv)
		pixel_bytes = 1;
		break;
	case RAW_FRAME_UNCOMPRESSED_YUV_ANY:
		// これは最大サイズを返す
		pixel_bytes = 4;
		break;
	default:
		LOGD("unexpected frame type 0x%08x, return 1 as pixel bytes", frame_type);
		break;
	}
	return pixel_bytes;
}

//================================================================================
/**
 * サイズ指定付きコンストラクタ
 * @param bytes
 */
BaseVideoFrame::BaseVideoFrame(const uint32_t &bytes)
:	BaseFrame(bytes),
	_frame_type(RAW_FRAME_UNKNOWN),
	_width(0),
	_height(0),
	_step(0),
	_pixelBytes(0),
	_pts(0), _stc(0), _sof(0), _packet_id(0), _frame_num_in_payload(1)
{
	ENTER();
	EXIT();
}

/**
 * フォーマット・サイズ指定付きコンストラクタ
 * @param width
 * @param height
 * @param frame_type
 */
BaseVideoFrame::BaseVideoFrame(
	const uint32_t &width, const uint32_t &height,
	const raw_frame_t &frame_type)
:	BaseVideoFrame(width, height, get_pixel_bytes(frame_type), frame_type)
{
	ENTER();
	EXIT();
}

/**
 * サイズ指定付きコンストラクタ
 * @param width
 * @param height
 * @param pixelBytes
 * @param frame_type
 */
BaseVideoFrame::BaseVideoFrame(
	const uint32_t &width, const uint32_t &height,
	const PixelBytes &pixelBytes,
	const raw_frame_t &frame_type)
:	BaseFrame(pixelBytes.frame_bytes(width, height)),
	_frame_type(frame_type),
	_width(width),
	_height(height),
	_step(pixelBytes.frame_bytes(width, 1)),
	_pixelBytes(pixelBytes),
	_pts(0), _stc(0), _sof(0), _packet_id(0), _frame_num_in_payload(1)
{
	ENTER();
	EXIT();
}

/**
 * コピーコンストラクタ
 * @param src
 */
BaseVideoFrame::BaseVideoFrame(const BaseVideoFrame &src)
:	BaseFrame(src),
	_frame_type(src._frame_type),
	_width(src._width),
	_height(src._height),
	_step(src._step),
	_pixelBytes(src._pixelBytes),
	_pts(src._pts), _stc(src._stc), _sof(src._sof),
	_packet_id(src._packet_id), _frame_num_in_payload(src._frame_num_in_payload)
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
BaseVideoFrame::~BaseVideoFrame() noexcept {
	ENTER();
	EXIT();
}

/**
 * 代入演算子(ディープコピー)
 * @param src
 * @return
 */
BaseVideoFrame &BaseVideoFrame::operator=(const BaseVideoFrame &src) {
	ENTER();

	BaseFrame::operator=(src);
	if (this != &src) {
		_frame_type = src._frame_type;
		_width = src._width;
		_height = src._height;
		_step = src._step;
		_pixelBytes = src._pixelBytes;
		_pts = src._pts;
		_stc = src._stc;
		_sof = src._sof;
		_packet_id = src._packet_id;
		_frame_num_in_payload = src._frame_num_in_payload;
	}

	RET(*this);
}

/**
 * 代入演算子(ディープコピー)
 * @param src
 * @return
 */
BaseVideoFrame &BaseVideoFrame::operator=(const IVideoFrame &src) {
	ENTER();
	
	auto _src = dynamic_cast<const BaseVideoFrame *>(&src);
	if (_src) {
		BaseFrame::operator=(*_src);
		if (this != &src) {
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
	}
	
	RET(*this);
}

/**
 * ディープコピー
 * @param dst
 * @return
 */
int BaseVideoFrame::copy_to(IVideoFrame &dst) const {
	ENTER();

	int result = USB_ERROR_NOT_SUPPORTED;
	auto _dst = dynamic_cast<BaseVideoFrame *>(&dst);
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
int BaseVideoFrame::get_image(VideoImage_t &image) {
	ENTER();
	RETURN(assign(image, frame_type(), width(), height(), frame(), actual_bytes()), int);
};

/**
 * 実際のバッファ自体を開放して保持しているデータサイズもクリアする
 * BaseFrame::recycleをoverride
 */
void BaseVideoFrame::recycle() {
	_width = _height = _step = 0;
	_pixelBytes = 0;
	BaseFrame::recycle();
}

/**
 * 映像サイズとフレームタイプを設定
 * @param new_width
 * @param new_height
 * @param new_frame_type
 */
void BaseVideoFrame::set_format(const uint32_t &width, const uint32_t &height,
	const raw_frame_t &frame_type) {

	resize(width, height, get_pixel_bytes(frame_type));
	_frame_type = frame_type;
}

/**
 * フレームタイプを設定
 * @param new_format
 */
void BaseVideoFrame::set_format(const raw_frame_t &frame_type) {
	resize(width(), height(), get_pixel_bytes(frame_type));
	_frame_type = frame_type;
}

/**
 * バッファサイズを変更
 * サイズが大きくなる時以外は実際のバッファのサイズ変更はしない
 * width, height, pixelBytes, stepは変更しない
 * IVideoFrameの純粋仮想関数を実装
 * @param bytes バッファサイズ
 * @return actual_bytes この#resizeだけactual_bytesを返すので注意, 他は成功したらUSB_SUCCESS(0), 失敗したらエラーコード
 */
size_t BaseVideoFrame::resize(const size_t &bytes) {
	return set_size(bytes);
}

/**
 * バッファサイズを変更
 * IVideoFrameの純粋仮想関数を実装
 * @param _width
 * @param _height
 * @param _pixel_bytes 1ピクセルあたりのバイト数, 0なら実際のバッファサイズの変更はせずにwidth, height等を更新するだけ
 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
 */
int BaseVideoFrame::resize(const uint32_t &new_width, const uint32_t &new_height,
	const PixelBytes &new_pixel_bytes) {

	ENTER();

	const auto bytes = new_pixel_bytes.frame_bytes(new_width, new_height);
	if (bytes) {
		set_size(bytes);
		if (UNLIKELY(size() < bytes)) {
			// 要求したのよりもバッファサイズが小さい時は確保できんかったということ
			RETURN(USB_ERROR_NO_MEM, int);
		}
	}
	_width = new_width;
	_height = new_height;
	_pixelBytes = new_pixel_bytes;
	// FIXME I420とかだと正しくない
	_step = new_pixel_bytes.frame_bytes(new_width, 1);

	RETURN(USB_SUCCESS, int);
}

/**
 * バッファサイズを変更
 * IVideoFrameの純粋仮想関数を実装
 * @param new_width
 * @param new_height
 * @param new_frame_type
 * @return
 */
int BaseVideoFrame::resize(const uint32_t &new_width, const uint32_t &new_height,
	const raw_frame_t &new_frame_type) {

	ENTER();

	const auto result = BaseVideoFrame::resize(new_width, new_height, get_pixel_bytes(new_frame_type));
	if (LIKELY(!result)) {
		_frame_type = new_frame_type;
	}

	RETURN(result, int);
}

/**
 * バッファサイズを変更
 * IVideoFrameの純粋仮想関数を実装
 * @param other widthと高さはこのVideoFrameインスタンスのものを使う
 * @param _frame_type フレームの種類(raw_frame_t)
 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
 */
int BaseVideoFrame::resize(const IVideoFrame &other, const raw_frame_t &new_frame_type) {
	ENTER();

	const auto result = resize(other.width(), other.height(), get_pixel_bytes(new_frame_type));
	if (LIKELY(!result)) {
		_frame_type = new_frame_type;
		set_attribute(other);
	}

	RETURN(result, int);
}

/**
 * 映像データの種類を取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
raw_frame_t BaseVideoFrame::frame_type() const {
	return _frame_type;
};

/**
 * 映像データの横幅(ピクセル数)を取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
uint32_t BaseVideoFrame::width() const {
	return _width;
};

/**
 * 映像データの高さ(ピクセル数)を取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
uint32_t BaseVideoFrame::height() const {
	return _height;
};

/**
 * 映像データ1行(横幅x1)分のバイト数を取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
uint32_t BaseVideoFrame::step() const {
	return _step;
};

/**
 * 1ピクセルあたりのバイト数を取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
const PixelBytes &BaseVideoFrame::pixel_bytes() const {
	return _pixelBytes;
};

/**
 * メージセンサーから映像を取得開始した時のクロック値
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
uint32_t BaseVideoFrame::pts() const {
	return _pts;
};

/**
 * 取得したイメージをUSBバスにセットし始めたときのクロック値
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
uint32_t BaseVideoFrame::stc() const {
	return _stc;
};

/**
 * stcを取得したときの１KHｚ SOFカウンタの値
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
uint32_t BaseVideoFrame::sof() const {
	return _sof;
};

/**
 * パケットIDを取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
size_t BaseVideoFrame::packet_id() const {
	return _packet_id;
};

/**
 * ペイロード中のマイクロフレームの数を取得
 * IVideoFrameの純粋仮想関数を実装
 * @return
 */
int BaseVideoFrame::frame_num_in_payload() const {
	return _frame_num_in_payload;
};

/**
 * ヘッダー情報を設定
 * IVideoFrameの純粋仮想関数を実装
 * @param pts
 * @param stc
 * @param sof
 * @return
 */
int BaseVideoFrame::set_header_info(
	const uint32_t &pts, const uint32_t &stc, const uint32_t &sof) {

	ENTER();

   	_pts = pts;
   	_stc = stc;
   	_sof = sof;

	RETURN(0, int);
}

/**
 * _presentationtime_us, _received_sys_time_us, sequence, _flags等の
 * 映像データ以外をコピーする
 * IVideoFrameの純粋仮想関数を実装
 * @param src
 */
void BaseVideoFrame::set_attribute(const IVideoFrame &src) {
	BaseFrame::set_attribute((const BaseFrame &) src);	// FIXME 本当はBaseFrame::set_attributeがIFrameを受け取るようにしないといけない
   	_stc = src.stc();
   	_sof = src.sof();
   	_packet_id = src.packet_id();
   	_frame_num_in_payload = src.frame_num_in_payload();
};

}	// namespace serenegiant::usb

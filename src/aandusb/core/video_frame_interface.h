/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_FRAME_INTERFACE_H
#define AANDUSB_VIDEO_FRAME_INTERFACE_H

#include <memory>

#include "core/video.h"
#include "core/video_image.h"
#include "core/frame_interface.h"

#pragma interface

namespace serenegiant::core {

class PixelBytes;
class BaseVideoFrame;

/**
 * 映像データ用IFrameインターフェースクラス(抽象クラス)
 */
class IVideoFrame : public virtual IFrame {
public:
	virtual ~IVideoFrame() noexcept {}

	/**
	 * フレームタイプ(映像データの種類)を取得
	 * @return
	 */
	virtual raw_frame_t frame_type() const = 0;
	/**
	 * 映像データの横幅(ピクセル数)を取得
	 * @return
	 */
	virtual uint32_t width() const = 0;
	/**
	 * 映像データの高さ(ピクセル数)を取得
	 * @return
	 */
	virtual uint32_t height() const = 0;
	/**
	 * 映像データ1行(横幅x1)分のバイト数を取得
	 * @return
	 */
	virtual uint32_t step() const = 0;
	/**
	 * 1ピクセルあたりのバイト数を取得
	 * @return
	 */
	virtual const PixelBytes &pixel_bytes() const = 0;
	/**
	 * メージセンサーから映像を取得開始した時のクロック値
	 * @return
	 */
	virtual uint32_t pts() const = 0;
	/**
	 * 取得したイメージをUSBバスにセットし始めたときのクロック値
	 * @return
	 */
	virtual uint32_t stc() const = 0;
    /**
     * stcを取得したときの１KHｚ SOFカウンタの値
	 * @return
     */
	virtual uint32_t sof() const = 0;
	/**
	 * パケットIDを取得
	 * @return
	 */
	virtual size_t packet_id() const = 0;
	/**
	 * ペイロード中のマイクロフレームの数を取得
	 * @return
	 */
	virtual int frame_num_in_payload() const = 0;

	/**
	 * バッファサイズを変更
	 * サイズが大きくなる時以外は実際のバッファのサイズ変更はしない
	 * width, height, pixelBytes, stepは変更しない
	 * @param bytes バッファサイズ
	 * @return actual_bytes この#resizeだけactual_bytesを返すので注意, 他は成功したらUSB_SUCCESS(0), 失敗したらエラーコード
	 */
	virtual size_t resize(const size_t &bytes) = 0;
	/**
	 * バッファサイズを変更
	 * @param new__width
	 * @param new__height
	 * @param new__pixel_bytes 1ピクセルあたりのバイト数, 0なら実際のバッファサイズの変更はせずにwidth, height等を更新するだけ
	 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
	 */
	virtual int resize(const uint32_t &new_width, const uint32_t &new_height,
		const PixelBytes &new_pixel_bytes) = 0;
	/**
	 * バッファサイズを変更
	 * @param new_width
	 * @param new_height
	 * @param new_frame_type
	 * @return
	 */
	virtual int resize(const uint32_t &new_width, const uint32_t &new_height,
		const raw_frame_t &new_frame_type) = 0;
	/**
	 * バッファサイズを変更
	 * @param other widthと高さはこのVideoFrameインスタンスのものを使う
	 * @param new__frame_type フレームの種類(raw_frame_t)
	 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
	 */
	virtual int resize(const IVideoFrame &other, const raw_frame_t &new_frame_type) = 0;

	/**
	 * ヘッダー情報を設定
	 * @param pts
	 * @param stc
	 * @param sof
	 * @return
	 */
	virtual int set_header_info(const uint32_t &pts, const uint32_t &stc, const uint32_t &sof) = 0;
	/**
	 * _presentationtime_us, _received_sys_time_us, sequence, _flags等の
	 * 映像データ以外をコピーする
	 * @param src
	 */
	virtual void set_attribute(const IVideoFrame &src) = 0;

	/**
	 * ディープコピー
	 * @param dst
	 * @return
	 */
	virtual int copy_to(IVideoFrame &dst) const = 0;

	/**
	 * 映像データをVideoImage_t形式で取得する
	 * @param image
	 * @return
	 */
	virtual int get_image(VideoImage_t &image) = 0;
};

typedef std::shared_ptr<IVideoFrame> IVideoFrameSp;
typedef std::unique_ptr<IVideoFrame> IVideoFrameUp;

}	// namespace serenegiant::usb

#endif //AANDUSB_VIDEO_FRAME_INTERFACE_H

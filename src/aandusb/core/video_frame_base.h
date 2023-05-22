/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_FRAME_BASE_H
#define AANDUSB_VIDEO_FRAME_BASE_H

#include <memory>

#include "core/video.h"
#include "core/frame_base.h"
#include "core/video_frame_interface.h"

namespace serenegiant::core {
/**
 * 1ピクセルあたりのバイト数を取得
 * @param format
 * @return
 */
PixelBytes get_pixel_bytes(const raw_frame_t &frame_type);

/**
 * 映像フレームデータ用BaseFrame & IVideoFrame実装
 */
class BaseVideoFrame : public virtual core::BaseFrame, public virtual IVideoFrame {
private:
	/** 映像データの種類 */
	raw_frame_t _frame_type;
	/** 映像データの横幅(ピクセル数) */
	uint32_t _width;
	/** 映像データの高さ(ピクセル数) */
	uint32_t _height;
	/** 映像データ1行(横幅x1)分のバイト数 */
	uint32_t _step;
	/** 1ピクセルあたりのバイト数 */
	PixelBytes _pixelBytes;
	/** ペイロードヘッダーのPresentation Time Stamp, イメージセンサーから映像を取得開始した時のクロック値 */
	uint32_t _pts;
	/**
	 * Source Time Clock, 取得したイメージをUSBバスにセットし始めたときのクロック値
	 * stcとsofをあわせてSCR(Source Clock Reference)
	 */
	uint32_t _stc;
	/**
	 * 1KHz SOF token counter, stcを取得したときの１KHｚ SOFカウンタの値
	 * stcとsofをあわせてSCR(Source Clock Reference)
	 */
	uint32_t _sof;
	/**
	 * アイソクロナス転送のパケットID、それ以外の時は0
	 */
	size_t _packet_id;
	/** ペイロード中のマイクロフレームの数 */
	int _frame_num_in_payload;
protected:
public:
	/**
	 * サイズ指定付きコンストラクタ
	 * @param bytes
	 */
	BaseVideoFrame(const uint32_t &bytes = 0);
	/**
	 * フォーマット・サイズ指定付きコンストラクタ
	 * @param width
	 * @param height
	 * @param frame_type
	 */
	BaseVideoFrame(
		const uint32_t &width, const uint32_t &height,
		const raw_frame_t &frame_type);
	/**
	 * サイズ指定付きコンストラクタ
	 * @param width
	 * @param height
	 * @param pixelBytes
	 */
	BaseVideoFrame(
		const uint32_t &width, const uint32_t &height,
		const PixelBytes &pixelBytes,
		const raw_frame_t &frame_type = RAW_FRAME_UNKNOWN);
	/**
	 * コピーコンストラクタ
	 * @param src
	 */
	BaseVideoFrame(const BaseVideoFrame &src);
	/**
	 * デストラクタ
	 */
	virtual ~BaseVideoFrame() noexcept;

	/**
	 * 代入演算子(ディープコピー)
	 * @param src
	 * @return
	 */
	BaseVideoFrame &operator=(const BaseVideoFrame &src);
	/**
	 * 代入演算子(ディープコピー)
	 * @param src
	 * @return
	 */
	BaseVideoFrame &operator=(const IVideoFrame &src);

	/**
	 * ディープコピー
	 * @param dst
	 * @return
	 */
	int copy_to(IVideoFrame &dst) const override;

	/**
	 * 映像データをVideoImage_t形式で取得する
	 * @param image
	 * @return
	 */
	int get_image(VideoImage_t &image) override;

	/**
	 * 実際のバッファ自体を開放して保持しているデータサイズもクリアする
	 * BaseFrame::recycleをoverride
	 */
	void recycle() override;

	/**
	 * 映像サイズとフレームタイプを設定
	 * @param new_width
	 * @param new_height
	 * @param new_frame_type
	 */
	void set_format(const uint32_t &new_width, const uint32_t &new_height,
		const raw_frame_t &new_frame_type);
	/**
	 * フレームタイプを設定
	 * @param new_format
	 */
	void set_format(const raw_frame_t &new_format);

	/**
	 * バッファサイズを変更
	 * サイズが大きくなる時以外は実際のバッファのサイズ変更はしない
	 * width, height, pixelBytes, stepは変更しない
	 * IVideoFrameの純粋仮想関数を実装
	 * @param bytes バッファサイズ
	 * @return actual_bytes この#resizeだけactual_bytesを返すので注意, 他は成功したらUSB_SUCCESS(0), 失敗したらエラーコード
	 */
	size_t resize(const size_t &bytes) override;
	/**
	 * バッファサイズを変更
	 * IVideoFrameの純粋仮想関数を実装
	 * @param new__width
	 * @param new__height
	 * @param new__pixel_bytes 1ピクセルあたりのバイト数, 0なら実際のバッファサイズの変更はせずにwidth, height等を更新するだけ
	 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
	 */
	int resize(const uint32_t &new_width, const uint32_t &new_height,
		const PixelBytes &new_pixel_bytes) override;
	/**
	 * バッファサイズを変更
	 * IVideoFrameの純粋仮想関数を実装
	 * @param new_width
	 * @param new_height
	 * @param new_frame_type
	 * @return
	 */
	int resize(const uint32_t &new_width, const uint32_t &new_height,
		const raw_frame_t &new_frame_type) override;
	/**
	 * バッファサイズを変更
	 * IVideoFrameの純粋仮想関数を実装
	 * @param other widthと高さはこのVideoFrameインスタンスのものを使う
	 * @param new__frame_type フレームの種類(raw_frame_t)
	 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
	 */
	int resize(const IVideoFrame &other, const raw_frame_t &new_frame_type) override;

	/**
	 * フレームタイプ(映像データの種類)を取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	raw_frame_t frame_type() const override;
	/**
	 * 映像データの横幅(ピクセル数)を取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	uint32_t width() const override;
	/**
	 * 映像データの高さ(ピクセル数)を取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	uint32_t height() const override;
	/**
	 * 映像データ1行(横幅x1)分のバイト数を取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	uint32_t step() const override;
	/**
	 * 1ピクセルあたりのバイト数を取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	const PixelBytes &pixel_bytes() const override;

	/**
	 * メージセンサーから映像を取得開始した時のクロック値
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	uint32_t pts() const override;
	/**
	 * 取得したイメージをUSBバスにセットし始めたときのクロック値
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
	uint32_t stc() const override;
    /**
     * stcを取得したときの１KHｚ SOFカウンタの値を取得
	 * IVideoFrameの純粋仮想関数を実装
     * @return
     */
	uint32_t sof() const override;
	/**
	 * パケットIDを設定
	 * @param packet_id
	 */
	inline void packet_id(const size_t &packet_id) { _packet_id = packet_id; };
	/**
	 * パケットIDを取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
    size_t packet_id() const override;
	/**
	 * ペイロード中のマイクロフレームの数を設定
	 * IVideoFrameの純粋仮想関数を実装
	 * @param frame_num_in_payload
	 */
    inline void frame_num_in_payload(const int &frame_num_in_payload) {
    	_frame_num_in_payload = frame_num_in_payload;
    };
	/**
	 * ペイロード中のマイクロフレームの数を取得
	 * IVideoFrameの純粋仮想関数を実装
	 * @return
	 */
    int frame_num_in_payload() const override;

	/**
	 * ヘッダー情報を設定
	 * IVideoFrameの純粋仮想関数を実装
	 * @param pts
	 * @param stc
	 * @param sof
	 * @return
	 */
	int set_header_info(const uint32_t &pts, const uint32_t &stc, const uint32_t &sof) override;
	/**
	 * _presentationtime_us, _received_sys_time_us, sequence, _flags等の
	 * 映像データ以外をコピーする
	 * IVideoFrameの純粋仮想関数を実装
	 * @param src
	 */
	void set_attribute(const IVideoFrame &src) override;
};

typedef std::shared_ptr<BaseVideoFrame> BaseVideoFrameSp;
typedef std::unique_ptr<BaseVideoFrame> BaseVideoFrameUp;

}	// namespace serenegiant::usb

#endif //AANDUSB_VIDEO_FRAME_BASE_H

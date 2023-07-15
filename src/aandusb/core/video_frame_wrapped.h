/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_FRAME_MMAPPED_H
#define AANDUSB_VIDEO_FRAME_MMAPPED_H

#include <memory>

#include "glutils.h"

#include "core/video.h"
#include "core/video_image.h"
#include "core/frame_base.h"
#include "core/video_frame_interface.h"
#include "core/video_frame_base.h"

namespace serenegiant::core {

/**
 * @brief 外部のメモリーをラップするためのIVideoFrame実装
 * 
 */
class WrappedVideoFrame : public virtual IVideoFrame {
private:
	uint8_t *buffer;
	size_t buffer_bytes;
	// IFrame実装用
	size_t _actual_bytes;
	nsecs_t _presentation_time_us;
	// 受信時のシステム時刻[マイクロ秒]保持用, #update_presentationtime_usを呼んだ時に設定される
	nsecs_t _received_sys_time_us;
	uint32_t _sequence;
	uint32_t _flags;
	uint32_t _option;

	// IVideoFrame実装用
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
	 * コンストラクタ
	 * @param buffer
	 * @param buffer_bytes
	 * @param width
	 * @param height
	 * @param frame_type
	 */
	WrappedVideoFrame(
		uint8_t *buffer, const size_t buffer_bytes,
		const uint32_t &width = 0, const uint32_t &height = 0,
		const raw_frame_t &frame_type = RAW_FRAME_UNKNOWN);

	/**
	 * デストラクタ
	 */
	virtual ~WrappedVideoFrame() noexcept;

	/**
	 * 代入演算子(ディープコピー)
	 * @param src
	 * @return
	 */
	WrappedVideoFrame &operator=(const WrappedVideoFrame &src);

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
	 * @brief 別の外部メモリーへ割り当て直す
	 * 
	 * @param buf 
	 * @param size 
	 * @param width
	 * @param height
	 * @param frame_type
	 */
	void assign(
		uint8_t *buf, const size_t &size,
		const uint32_t &width, const uint32_t &height,
		const raw_frame_t &frame_type);
	//--------------------------------------------------------------------------------
	// IFrameの純粋仮想関数の実装
	/**
	 * 配列添え字演算子
	 * プレフィックス等を考慮した値を返す
	 * @param ix
	 * @return
	 */
	uint8_t &operator[](uint32_t ix) override;
	/**
	 * 配列添え字演算子
	 * プレフィックス等を考慮した値を返す
	 * @param ix
	 * @return
	 */
	const uint8_t &operator[](uint32_t ix) const override;
	/**
	 * 保持しているデータサイズを取得
	 * プレフィックス等を考慮した値を返す
	 * (バッファの実際のサイズではない)
	 * #actual_bytes <= #raw_bytes <= #size
	 * @return
	 */
	size_t actual_bytes() const override;
	/**
	 * バッファサイズを取得(バッファを拡張せずに保持できる最大サイズ)
	 * #actual_bytes <= #raw_bytes <= #size
	 * @return
	 */
	size_t size() const override;
	/**
	 * フレームバッファーの先頭ポインタを取得
	 * プレフィックス等を考慮した値を返す
	 * @return
	 */
	uint8_t *frame() override;
	/**
	 * フレームバッファーの先頭ポインタを取得
	 * プレフィックス等を考慮した値を返す
	 * @return
	 */
	const uint8_t *frame() const override;
	/**
	 * フレームデータのPTS[マイクロ秒]をを取得
	 * @return
	 */
	nsecs_t presentation_time_us() const override;
	/**
	 * フレームデータ受信時のシステム時刻[マイクロ秒]を取得する
	 * @return
	 */
	nsecs_t  received_sys_time_us() const override;
	/**
	 * フレームシーケンス番号を取得
	 * @return
	 */
	uint32_t sequence() const override;
	/**
	 * フレームのフラグを取得
	 * @return
	 */
	uint32_t flags() const override;
	/**
	 * フレームのオプションフラグを取得
	 * @return
	 */
	uint32_t option() const override;
	/**
	 * 保持しているデータサイズをクリアする, 実際のバッファ自体はそのまま
	 */
	void clear() override;

	//--------------------------------------------------------------------------------
	// IVideoFrameの純粋仮想関数の実装
	/**
	 * フレームタイプ(映像データの種類)を取得
	 * @return
	 */
	raw_frame_t frame_type() const override;
	/**
	 * 映像データの横幅(ピクセル数)を取得
	 * @return
	 */
	uint32_t width() const override;
	/**
	 * 映像データの高さ(ピクセル数)を取得
	 * @return
	 */
	uint32_t height() const override;
	/**
	 * 映像データ1行(横幅x1)分のバイト数を取得
	 * @return
	 */
	uint32_t step() const override;
	/**
	 * 1ピクセルあたりのバイト数を取得
	 * @return
	 */
	const PixelBytes &pixel_bytes() const override;
	/**
	 * メージセンサーから映像を取得開始した時のクロック値
	 * @return
	 */
	uint32_t pts() const override;
	/**
	 * 取得したイメージをUSBバスにセットし始めたときのクロック値
	 * @return
	 */
	uint32_t stc() const override;
    /**
     * stcを取得したときの１KHｚ SOFカウンタの値
	 * @return
     */
	uint32_t sof() const override;
	/**
	 * パケットIDを取得
	 * @return
	 */
	size_t packet_id() const override;
	/**
	 * ペイロード中のマイクロフレームの数を取得
	 * @return
	 */
	int frame_num_in_payload() const override;

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
	 * @param bytes バッファサイズ
	 * @return actual_bytes この#resizeだけactual_bytesを返すので注意, 他は成功したらUSB_SUCCESS(0), 失敗したらエラーコード
	 */
	size_t resize(const size_t &bytes) override;
	/**
	 * バッファサイズを変更
	 * @param new__width
	 * @param new__height
	 * @param new__pixel_bytes 1ピクセルあたりのバイト数, 0なら実際のバッファサイズの変更はせずにwidth, height等を更新するだけ
	 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
	 */
	int resize(const uint32_t &new_width, const uint32_t &new_height,
		const PixelBytes &new_pixel_bytes) override;
	/**
	 * バッファサイズを変更
	 * @param new_width
	 * @param new_height
	 * @param new_frame_type
	 * @return
	 */
	int resize(const uint32_t &new_width, const uint32_t &new_height,
		const raw_frame_t &new_frame_type) override;
	/**
	 * バッファサイズを変更
	 * @param other widthと高さはこのVideoFrameインスタンスのものを使う
	 * @param new__frame_type フレームの種類(raw_frame_t)
	 * @return サイズ変更できればUSB_SUCCESS(0), それ以外ならエラー値を返す
	 */
	int resize(const IVideoFrame &other, const raw_frame_t &new_frame_type) override;

	/**
	 * ヘッダー情報を設定
	 * @param pts
	 * @param stc
	 * @param sof
	 * @return
	 */
	int set_header_info(const uint32_t &pts, const uint32_t &stc, const uint32_t &sof) override;
	/**
	 * _presentationtime_us, _received_sys_time_us, sequence, _flags等の
	 * 映像データ以外をコピーする
	 * @param src
	 */
	void set_attribute(const IVideoFrame &src) override;
};

typedef std::shared_ptr<WrappedVideoFrame> WrappedVideoFrameSp;
typedef std::unique_ptr<WrappedVideoFrame> WrappedVideoFrameUp;

}   // namespace serenegiant::core

#endif // AANDUSB_VIDEO_FRAME_MMAPPED_H

/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_FRAME_BASE_H
#define AANDUSB_FRAME_BASE_H

#include <vector>
// core
#include "core/frame_interface.h"

namespace serenegiant::core {

/**
 * フレームデータの基底クラス
 * IFrame純粋仮想クラスを実装
 */
class BaseFrame : public virtual IFrame {
private:
	size_t _actual_bytes;
	nsecs_t _presentation_time_us;
	// 受信時のシステム時刻[マイクロ秒]保持用, #update_presentationtime_usを呼んだ時に設定される
	nsecs_t _received_sys_time_us;
	uint32_t _sequence;
	uint32_t _flags;
	uint32_t _option;
	std::vector<uint8_t> _frame;
protected:
	/**
	 * サイズ指定付きコンストラクタ
	 * @param bytes
	 */
	BaseFrame(const uint32_t &bytes = 0);
	/**
	 * コピーコンストラクタ
	 * @param src
	 */
	BaseFrame(const BaseFrame &src);
public:
	/**
	 * デストラクタ
	 */
	virtual ~BaseFrame() noexcept;

	/**
	 * 代入演算子(ディープコピー)
	 * @param src
	 * @return
	 */
	BaseFrame &operator=(const BaseFrame &src);
	/**
	 * 代入演算子(ディープコピー)
	 * @param src
	 * @return
	 */
	BaseFrame &operator=(const IFrame &src);

	/**
	 * プレフィックス等を考慮しない保持しているデータサイズを取得
	 * #actual_bytes <= #raw_bytes <= #size
	 * @return
	 */
	inline const size_t raw_bytes() const { return _actual_bytes; };
	/**
	 * プレフィックス等を考慮しないフレームバッファの先頭からixバイト位置のポインターを取得
	 * @param ix デフォルト0
	 * @return
	 */
	inline uint8_t *raw_frame(const uint32_t ix = 0) { return &_frame[ix]; };
	/**
	 * プレフィックス等を考慮しないフレームバッファの先頭からixバイト位置のポインターを取得
	 * @param ix デフォルト0
	 * @return
	 */
	inline const uint8_t *raw_frame(const uint32_t ix = 0) const { return &_frame[ix]; };

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
	 * 実際のバッファ自体を開放して保持しているデータサイズもクリアする
	 */
	virtual void recycle();
	/**
	 * 保持しているデータサイズをクリアする, 実際のバッファ自体はそのまま
	 */
	virtual void clear() override;

	/**
	 * バッファサイズを変更
	 * サイズが大きくなる時以外は実際のバッファのサイズ変更はしない
	 * @param bytes バッファサイズ
	 * @return actual_bytes
	 */
	size_t set_size(const size_t &new_bytes);

	/**
	 * 必要に応じてバッファサイズを変更してフレームデータをコピーする
	 * @param data
	 * @param bytes
	 */
	void setFrame(const uint8_t *data, const size_t &bytes);

	/**
	 * 現在のバッファの最後に新しいデータを追加する
	 * バッファサイズが足らないときはバッファサイズを拡張する
	 * @param data
	 * @param bytes
	 * @return 0: 追加に成功した それ以外: 追加できなかった
	 */
	int append(const uint8_t *data, const size_t &bytes);

	/**
	 * フレームデータ受信時のシステム時刻[マイクロ秒]を取得する
	 * ::update_presentationtime_usを呼んだ時に設定される
	 * @return
	 */
	nsecs_t received_sys_time_us() const override;
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
	 *　フレームデータのPTSを設定
	 * @param sequence
	 * @param presentationtime_us 0の場合は呼び出し時のシステム時刻[マイクロ秒]がセットされる
	 * @return
	 */
	inline const nsecs_t &update_presentationtime_us(
		const uint32_t &sequence, const nsecs_t &presentationtime_us = 0) {

		_sequence = sequence;
		_received_sys_time_us = systemTime() / 1000LL;
		if (presentationtime_us) {
			_presentation_time_us = presentationtime_us;
		} else {
			_presentation_time_us = _received_sys_time_us;
		}
		return _presentation_time_us;
	};
	/**
	 * フレームデータのPTSをを設定
	 * @param presentationtime_us
	 */
	inline void presentationtime_us(const nsecs_t &presentationtime_us) {
		_presentation_time_us = presentationtime_us;
	};

	/**
	 * _presentationtime_us, _received_sys_time_us, sequence, _flags等の
	 * 映像データ以外をコピーする
	 * @param src
	 */
	inline void set_attribute(const IFrame &src) {
		_presentation_time_us = src.presentation_time_us();
		_received_sys_time_us = src.received_sys_time_us();
		_sequence = src.sequence();
		_flags = src.flags();
		_option = src.option();
	}

	/**
	 * フラグをセット
	 * @param flags
	 */
	inline void flags(const uint32_t &flags) { _flags = flags; };
	/**
	 * オプションフラグをセット
	 * @param option
	 */
	inline void option(const uint32_t &option) { _option = option; };
};

}	// namespace serenegiant::usb {

#endif //AANDUSB_FRAME_BASE_H

/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_FRAME_INTERFACE_H
#define AANDUSB_FRAME_INTERFACE_H

#include <memory>

#include "times.h"

namespace serenegiant::core {

/**
 * フレームデータ用インターフェースクラス(抽象クラス)
 */
class IFrame {
public:
	virtual ~IFrame() noexcept {};

	/**
	 * 配列添え字演算子
	 * プレフィックス等を考慮した値を返す
	 * @param ix
	 * @return
	 */
	virtual uint8_t &operator[](uint32_t ix) = 0;
	/**
	 * 配列添え字演算子
	 * プレフィックス等を考慮した値を返す
	 * @param ix
	 * @return
	 */
	virtual const uint8_t &operator[](uint32_t ix) const = 0;
	/**
	 * 保持しているデータサイズを取得
	 * プレフィックス等を考慮した値を返す
	 * (バッファの実際のサイズではない)
	 * #actual_bytes <= #raw_bytes <= #size
	 * @return
	 */
	virtual size_t actual_bytes() const = 0;
	/**
	 * バッファサイズを取得(バッファを拡張せずに保持できる最大サイズ)
	 * #actual_bytes <= #raw_bytes <= #size
	 * @return
	 */
	virtual size_t size() const = 0;
	/**
	 * フレームバッファーの先頭ポインタを取得
	 * プレフィックス等を考慮した値を返す
	 * @return
	 */
	virtual uint8_t *frame() = 0;
	/**
	 * フレームバッファーの先頭ポインタを取得
	 * プレフィックス等を考慮した値を返す
	 * @return
	 */
	virtual const uint8_t *frame() const = 0;
	/**
	 * フレームデータのPTS[マイクロ秒]をを取得
	 * @return
	 */
	virtual nsecs_t presentation_time_us() const = 0;
	/**
	 * フレームデータ受信時のシステム時刻[マイクロ秒]を取得する
	 * @return
	 */
	virtual nsecs_t  received_sys_time_us() const = 0;
	/**
	 * フレームシーケンス番号を取得
	 * @return
	 */
	virtual uint32_t sequence() const = 0;
	/**
	 * フレームのフラグを取得
	 * @return
	 */
	virtual uint32_t flags() const = 0;
	/**
	 * フレームのオプションフラグを取得
	 * @return
	 */
	virtual uint32_t option() const = 0;
	/**
	 * 保持しているデータサイズをクリアする, 実際のバッファ自体はそのまま
	 */
	virtual void clear() = 0;
};

typedef std::shared_ptr<IFrame> IFrameSp;
typedef std::unique_ptr<IFrame> IFrameUp;

}	// namespace serenegiant::usb

#endif //AANDUSB_FRAME_INTERFACE_H

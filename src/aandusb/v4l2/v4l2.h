/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_V4L2_H
#define AANDUSB_V4L2_H

#include <memory>

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

namespace usb = serenegiant::usb;
namespace uvc = serenegiant::usb::uvc;

namespace serenegiant::v4l2 {

/**
 * オープン
 *     -> 解像度・ピクセルフォーマット設定
 *     -> ストリーム開始
 *         -> [解像度・ピクセルフォーマット設定]
 *     -> ストリーム停止
 * -> クローズ
 */
typedef enum _state {
	STATE_CLOSE = 0,	// openされていない
	STATE_OPEN,			// openされたが解像度が選択されていない
	STATE_INIT,			// open&解像度選択済み
	STATE_STREAM,		// カメラ映像取得中
} state_t;

typedef std::shared_ptr<struct v4l2_queryctrl> QueryCtrlSp;
typedef std::unique_ptr<struct v4l2_queryctrl> QueryCtrlUp;

} // namespace serenegiant::v4l2

#endif // AANDUSB_V4L2_H
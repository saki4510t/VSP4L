/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_V4L2_H
#define AANDUSB_V4L2_H

#include <memory>
#include <string>
#include <unordered_map>

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

// common
#include "json_helper.h"
// uvc
#include "uvc/aanduvc.h"

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

typedef struct _buffer {
	void *start;
	size_t length;
} buffer_t;

typedef std::shared_ptr<struct v4l2_queryctrl> QueryCtrlSp;
typedef std::unique_ptr<struct v4l2_queryctrl> QueryCtrlUp;

typedef std::shared_ptr<struct v4l2_frmivalenum> V4L2FrameIntervalSp;
typedef std::unique_ptr<struct v4l2_frmivalenum> V4L2FrameIntervalUp;

typedef struct _frame_info {
	const int index;
	const uint32_t pixel_format;
	const uint32_t type;
	uint32_t width;
	uint32_t height;
	uint32_t min_width;
	uint32_t max_width;
	uint32_t step_width;
	uint32_t min_height;
	uint32_t max_height;
	uint32_t step_height;
	std::vector<V4L2FrameIntervalSp> frame_rates;

	_frame_info(const int &index, const uint32_t &pixel_format, const uint32_t &type)
	:	index(index), pixel_format(pixel_format), type(type) { }
} frame_info_t;

typedef std::shared_ptr<frame_info_t> FrameInfoSp;
typedef std::unique_ptr<frame_info_t> FrameInfoUp;

typedef struct _format_info {
	const int index;
	const uint32_t pixel_format;
	std::vector<FrameInfoSp> frames;

	_format_info(const int &index, const uint32_t &pixel_format)
	:	index(index), pixel_format(pixel_format) { }
} format_info_t;

typedef std::shared_ptr<format_info_t> FormatInfoSp;
typedef std::unique_ptr<format_info_t> FormatInfoUp;

std::string V4L2_PIX_FMT_to_string(const uint32_t &v4l2_pix_fmt);
core::raw_frame_t V4L2_PIX_FMT_to_raw_frame(const uint32_t &v4l2_pix_fmt);
uint32_t raw_frame_to_V4L2_PIX_FMT(const core::raw_frame_t &frame_type);
/**
 * ioctl呼び出し中にシグナルを受け取るとエラー終了してしまうので
 * シグナル受信時(errno==EINTR)ならリトライするためのヘルパー関数
 * @param fd
 * @param request
 * @param arg
 * @return
 */
int xioctl(int fd, int request, void *arg);

const char *ctrl_type_string(const uint32_t &type);

/**
 * コントロール機能がメニュータイプの場合の設定項目値をログ出力する
 * @param fd
 * @param query
 */
void list_menu_ctrl(int fd, const struct v4l2_queryctrl &query);

/**
 * v4l2_queryctrlをログへ出力
 * @param query
 */
void dump_ctrl(int fd, const struct v4l2_queryctrl &query);

/**
 * 対応するコントロール機能一覧をマップに登録する
 * @param fd
 * @param supported
 */
void update_ctrl_all_locked(int fd, std::unordered_map<uint32_t, QueryCtrlSp> &supported);

/**
 * jsonへフレームレート設定を出力するためのヘルパー関数
 * @param writer
 * @param frames
 * @param max_width
 * @param max_height
 */
void write_fps(Writer<StringBuffer> &writer,
	const std::vector<FrameInfoSp> &frames,
	const int &max_width = -1, const int &max_height = -1);

/**
 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
 * ::get_supported_size, ::get_supported_frameの下請け
 * @param fd
 * @param frame
 */
void get_supported_fps(int fd, const FrameInfoSp &frame);

/**
 * 対応するピクセルフォーマット・解像度・フレームレートをjson文字列として取得する
 * ::get_supported_sizeの下請け
 * @param fd
 * @param format
 */
void get_supported_frame_size(int fd, FormatInfoSp &format);

} // namespace serenegiant::v4l2

#endif // AANDUSB_V4L2_H
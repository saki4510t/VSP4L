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
#include <vector>

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
	int fd;
	void *start;
	size_t offset;
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

/**
 * コントロール機能をIDを文字列に変換
 * @param id
 * @param const char *
*/
const char *ctrl_id_string(const uint32_t &id);
/**
 * コントロール機能の種類を文字列に変換
 * @param type
 * @param const char *
*/
const char *ctrl_type_string(const uint32_t &type);

/**
 * コントロール機能がメニュータイプの場合の設定項目値をログ出力する
 * @param fd V4L2機器のファイルディスクリプタ
 * @param query
 */
void list_menu_ctrl(int fd, const struct v4l2_queryctrl &query);

/**
 * v4l2_queryctrlをログへ出力
 * @param fd V4L2機器のファイルディスクリプタ
 * @param query
 */
void dump_ctrl(int fd, const struct v4l2_queryctrl &query);

/**
 * コントロール機能がメニュータイプの場合の設定項目値を取得する
 * @param fd V4L2機器のファイルディスクリプタ
 * @param query
 * @param 設定項目値をセットするstd::vector<std::string>
 */
int get_menu_items(int fd, const struct v4l2_queryctrl &query, std::vector<std::string> &items);

/**
 * 対応するコントロール機能一覧をマップに登録する
 * @param fd
 * @param supported
 * @param dump 見つかったコントロール機能をログへ出力するかどうか, デフォルト=false(ログへ出力しない)
 */
void update_ctrl_all_locked(int fd, std::unordered_map<uint32_t, QueryCtrlSp> &supported,  const bool &dump = false);

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

/**
 * 指定したピクセルフォーマット・解像度・フレームレートに対応しているかどうかを確認
 * ::find_stream, ::find_frameの下請け
 * @param fd
 * @param pixel_format
 * @param width
 * @param height
 * @param min_fps
 * @param max_fps
 * @return
 */
int find_fps(int fd,
	const uint32_t &pixel_format,
	const uint32_t &width, const uint32_t &height,
	const float &min_fps, const float &max_fps);

/**
 * 指定したピクセルフォーマット・解像度・フレームレートに対応しているかどうかを確認
 * ::find_streamの下請け
 * @param fd
 * @param pixel_format
 * @param width
 * @param height
 * @param min_fps
 * @param max_fps
 * @return 0: 対応している, 0以外: 対応していない
 */
int find_frame_size(int fd,
	const uint32_t &pixel_format,
	const uint32_t &width, const uint32_t &height,
	const float &min_fps, const float &max_fps);

/**
 * 指定したピクセルフォーマットのフレームサイズ設定の数を取得する
 * @param fd
 * @param pixel_format
 * @return VIDIOC_ENUM_FRAMESIZES呼び出しが成功した回数
*/
int get_frame_size_nums(int fd, const uint32_t &pixel_format);

} // namespace serenegiant::v4l2

#endif // AANDUSB_V4L2_H
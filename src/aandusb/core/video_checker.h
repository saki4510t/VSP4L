/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_CHECKER_H
#define AANDUSB_VIDEO_CHECKER_H

#include <turbojpeg.h>
// core
#include "core/video_frame_interface.h"
#include "core/video_frame_utils.h"

namespace serenegiant::core {

class VideoChecker {
private:
	tjhandle jpegDecompressor;
	int init_jpeg_turbo();
	int mjpeg_check_header(core::IVideoFrame &in);
protected:
public:
	/**
	 * コンストラクタ
	 */
	VideoChecker();
	/**
	 * デストラクタ
	 */
	~VideoChecker();

	/**
	 * フレームヘッダーをチェックする
	 * 今のところチェックできるのはMJPEGのみ
	 * @param in
	 * @return	USB_SUCCESS:正常,
	 *			USB_ERROR_REALLOC:フレームサイズが異なっていたが実際のサイズに修正した,
	 *			その他:修正しようとしたがメモリが足りなかった・思っているのとフレームフォーマットが違ったなど
	 */
	int check_header(IVideoFrame &in);
	/**
	 * サニタリーチェック、
	 * MJPEGのみ、ヘッダーのチェックに加えてJFIFマーカーを手繰ってフォーマットが正しいかどうかを確認
	 * @param in
	 * @return	USB_SUCCESS:正常
	 *			USB_ERROR_REALLOC:フレームサイズが異なっていたが実際のサイズに修正した,
	 *			その他:修正しようとしたがメモリが足りなかった・思っているのとフレームフォーマットが違ったなど
	 */
	int check_sanitary(IVideoFrame &in);
	/**
	 * フレームデータがAnnexBマーカーで開始しているかどうかを確認
	 * @param in
	 * @return
	 */
	static bool start_with_annexb(IVideoFrame &in);
	/**
	 * I-Frameかどうかをチェック
	 * @param in
	 * @return I-Frameならtrue
	 * */
	static bool is_iframe(IVideoFrame &in);
	/**
	 * annexBフォーマットの時に指定した位置からnalユニットを探す
	 * 最初に見つかったnalユニットの先頭オフセットをoffsetに返す
	 * annexBフォーマットでない時、見つからない時は負を返す
	 * 見つかった時はnal_unit_type_tのいずれかを返す
	 * @param in
	 * @param offset
	 * @return
	 */
	static int nal_unit_type(IVideoFrame &in, size_t &offset);
};

} // namespace serenegiant::usb

#endif //AANDUSB_VIDEO_CHECKER_H

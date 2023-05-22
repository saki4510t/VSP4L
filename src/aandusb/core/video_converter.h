/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_CONVERTER_H
#define AANDUSB_VIDEO_CONVERTER_H

#include <vector>

#include <turbojpeg.h>
// core
#include "core/video_frame_interface.h"
#include "core/video_frame_utils.h"

namespace serenegiant::core {

class VideoConverter {
private:
	tjhandle jpegDecompressor;
	dct_mode_t _dct_mode;	// デフォルトはDEFAULT_DCT_MODE==DCT_MODE_IFAST
	std::vector<uint8_t> _work1;
	std::vector<uint8_t> _work2;
	/**
	 * libjpeg-turboを(m)jpeg展開用に初期化する
	 * 既に初期化済みの場合はなにもしない
	 * @return
	 */
	int init_jpeg_turbo();
protected:
public:
	/**
	 * コンストラクタ
	 * @param dct_mode
	 */
	VideoConverter(const dct_mode_t &dct_mode = DEFAULT_DCT_MODE);
	/**
	 * コピーコンストラクタ
	 * @param src
	 */
	VideoConverter(const VideoConverter &src);
	/**
	 * デストラクタ
	 */
	virtual ~VideoConverter();

	/**
	 * mjpegフレームデータをデコードした時のフレームタイプを取得
	 * 指定したIVideoFrameがmjpegフレームでないときなどはRAW_FRAME_UNKNOWNを返す
	 * @param src
	 * @return
	 */
	raw_frame_t get_mjpeg_decode_type(const IVideoFrame &src);

	/**
	 * 映像データをコピー
	 * 必要であれば映像フォーマットの変換を行う
	 * FIXME yuv系からJPEGへの圧縮など対応していない変換もあるので注意！(未対応時はUSB_ERROR_NOT_SUPPORTEDを返す)
	 * @param src
	 * @param dst
	 * @param dst_type コピー先の映像フォーマット, 指定しなければRAW_FRAME_UNKNOWNで単純コピーになる
	 * @return
	 */
	int copy_to(
		const IVideoFrame &src,
		IVideoFrame &dst,
		const raw_frame_t &dst_type = RAW_FRAME_UNKNOWN);

	/**
	 * 映像データをコピー
	 * 必要であれば映像フォーマットの変換を行う
	 * FIXME yuv系からJPEGへの圧縮など対応していない変換もあるので注意！(未対応時はUSB_ERROR_NOT_SUPPORTEDを返す)
	 * @param src
	 * @param dst
	 * @param dst_type
	 * @return
	 */
	int copy_to(
		IVideoFrame &src,
		VideoImage_t &dst);

	inline void set(const dct_mode_t &mode) {
		_dct_mode = mode;
	}

	inline void set(const VideoConverter &other) {
		_dct_mode = other._dct_mode;
	}

	inline const dct_mode_t &dct_mode() const { return _dct_mode; };
};

} // namespace serenegiant::usb

#endif //AANDUSB_VIDEO_CONVERTER_H

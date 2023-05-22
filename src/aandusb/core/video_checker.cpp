/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
//	#define USE_LOGALL
//	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include "utilbase.h"
// core
#include "core/video_checker.h"
// usb
#include "usb/aandusb.h"
// uvc
#include "uvc/aanduvc.h"

namespace serenegiant::core {

/**
 * MJPEGのヘッダーをチェックしてサイズを調整する
 * @param jpegDecompressor
 * @param in
 * @return	USB_SUCCESS:正常(フレームサイズが異なっていたが実際のサイズに修正した時も含む)
 *			その他:修正しようとしたがメモリが足りなかった・思っているのとフレームフォーマットが違ったなど
 */
static int _mjpeg_check_header(tjhandle &jpegDecompressor, IVideoFrame &in) {

	if (UNLIKELY((in.frame_type() != RAW_FRAME_MJPEG) || !jpegDecompressor)) {
		RETURN(USB_ERROR_INVALID_PARAM, int);
	}

	size_t jpeg_bytes;
	int sub_samp, width, height;
	int result = parse_mjpeg_header(jpegDecompressor, in, jpeg_bytes, sub_samp, width, height);
	if (LIKELY(!result)) {
		if (UNLIKELY((width != in.width()) || (height != in.height()))) {
			LOGW("frame size is different from actual mjpeg size, resizing %dx%d => %dx%d",
				in.width(), in.height(), width, height);
			if (UNLIKELY(in.resize((uint32_t)width, (uint32_t)height, RAW_FRAME_MJPEG))) {
				result = USB_ERROR_NO_MEM;
			} else {
				result = USB_SUCCESS;
			}
		}
	} else {
		LOGW("wrong header,err=%d,actualBytes=%zu", result, in.actual_bytes());
		result = USB_ERROR_NOT_SUPPORTED;
	}

	RETURN(result, int);
}

//--------------------------------------------------------------------------------
/**
 * コンストラクタ
 */
VideoChecker::VideoChecker()
: jpegDecompressor(nullptr)
{
	ENTER();
	EXIT();
}

/**
 * デストラクタ
 */
VideoChecker::~VideoChecker() {
	ENTER();
	if (jpegDecompressor) {
		tjDestroy(jpegDecompressor);
		jpegDecompressor = nullptr;
	}
	EXIT();
}

/*private*/
int VideoChecker::init_jpeg_turbo() {
	ENTER();

	int result = USB_SUCCESS;
	if (UNLIKELY(!jpegDecompressor)) {
		jpegDecompressor = tjInitDecompress();
		LOGD("tjInitDecompress");
		if (!jpegDecompressor) {
			const char *err = tjGetErrorStr();
			if (err) {
				LOGE("failed to call tjInitDecompress:%s", err);
			} else {
				LOGE("failed to call tjInitDecompress:unknown err");
			}
			result = USB_ERROR_OTHER;
		}
	}

	RETURN(result, int);
}

/**
 * MJPEGのヘッダーチェック
 * @param in
 * @return	USB_SUCCESS:正常(フレームサイズが異なっていたが実際のサイズに修正した時も含む)
 *			その他:修正しようとしたがメモリが足りなかった・思っているのとフレームフォーマットが違ったなど
 */
int VideoChecker::mjpeg_check_header(IVideoFrame &in) {
	ENTER();

	int result = init_jpeg_turbo();
	if (LIKELY(!result)) {
		result = _mjpeg_check_header(jpegDecompressor, in);
	}

	RETURN(result, int);
}

/**
 * フレームヘッダーをチェックする
 * 今のところチェックできるのはMJPEGのみ
 * @param in
 * @return	USB_SUCCESS:正常(フレームサイズが異なっていたが実際のサイズに修正した時も含む)
 *			その他:修正しようとしたがメモリが足りなかった・思っているのとフレームフォーマットが違ったなど
 */
int VideoChecker::check_header(IVideoFrame &in) {
	ENTER();

	int result;
	switch (in.frame_type()) {
	case RAW_FRAME_MJPEG:
		result = mjpeg_check_header(in);
		break;
	case RAW_FRAME_H264:
	case RAW_FRAME_FRAME_H264:
		// annexB形式で始まっているかどうかのチェックだけ
		result = start_with_annexb(in) ? USB_SUCCESS : VIDEO_ERROR_FRAME;
		break;
//	case RAW_FRAME_VP8:
//	case RAW_FRAME_FRAME_VP8:
//	case RAW_FRAME_H265:				/** H.265単独フレーム */
	default:
		result = USB_SUCCESS;
		break;
	}

	RETURN(result, int);
}

/**
 * サニタリーチェック、
 * MJPEGのみ、ヘッダーのチェックに加えてJFIFマーカーを手繰ってフォーマットが正しいかどうかを確認
 * @param in
 * @return	USB_SUCCESS:正常
 *			USB_ERROR_REALLOC:フレームサイズが異なっていたが実際のサイズに修正した,
 *			その他:修正しようとしたがメモリが足りなかった・思っているのとフレームフォーマットが違ったなど
 */
int VideoChecker::check_sanitary(IVideoFrame &in) {
	ENTER();

	int result = check_header(in);
	switch (in.frame_type()) {
	case RAW_FRAME_MJPEG:
		if (!result || (result == VIDEO_ERROR_FRAME)) {
			result = sanitaryMJPEG(in, in.frame(), in.actual_bytes());
		}
		break;
//	case RAW_FRAME_H264:
//	case RAW_FRAME_FRAME_H264:
//	case RAW_FRAME_VP8:
//	case RAW_FRAME_FRAME_VP8:
	default:
		// do nothing now
		break;
	}
	RETURN(result, int);
}

/**
 * N[00] 00 00 01(N>=0)で始まる位置を探す
 * 見つかれば0, 見つからなければ-1を返す
 * payloadがnullptrでなければannexbマーカーの次の位置を*payloadにセットする
 * XXX media_utils.cppにも同じ関数があるけどmedia_utils.cppはAndroidでしかビルドしないのでここでも定義しておく
 */
static int find_annexb(const uint8_t *data, const size_t &len, const uint8_t **payload) {
	ENTER();

	if (payload) {
		*payload = nullptr;
	}
	for (int i = 0; i < len - 4; i++) {	// 本当はlen-3までだけどpayloadが無いのは無効とみなしてlen-4までとする
		// 最低2つは連続して0x00でないとだめ
		if ((data[0] != 0x00) || (data[1] != 0x00)) {
			data++;
			continue;
		}
		// 3つ目が0x01ならOK
		if (data[2] == 0x01) {
			if (payload) {
				*payload = data + 3;
			}
			RETURN(0, int);
		}
		data++;
	}
	RETURN(-1, int);
}

/**
 * フレームデータがAnnexBマーカーで開始しているかどうかを確認
 * @param in
 * @return
 */
bool VideoChecker::start_with_annexb(IVideoFrame &in) {
	ENTER();

	bool result = false;
	const size_t _actual_bytes = in.actual_bytes();
	if (((RAW_FRAME_H264 == in.frame_type()) || (RAW_FRAME_FRAME_H264 == in.frame_type()))
		&& (_actual_bytes > 3)) {
		const uint8_t *data = in.frame();
		const uint8_t *payload = nullptr;
		result = !find_annexb(data, _actual_bytes, &payload);
	}

	RETURN(result, bool);
}

/**
 * I-Frameかどうかをチェック
 * @param in
 * @return I-Frameならtrue
 * */
bool VideoChecker::is_iframe(IVideoFrame &in) {
	ENTER();

	bool result = false;
	const size_t _actual_bytes = in.actual_bytes();
	if (((RAW_FRAME_H264 == in.frame_type()) || (RAW_FRAME_FRAME_H264 == in.frame_type()))
		&& (_actual_bytes > 3)) {
		const uint8_t *data = in.frame();
		const uint8_t *payload = nullptr;
		size_t sz = _actual_bytes;
		LOGD("先頭がannexbマーカーで始まっているかどうかを確認");
		int ret = find_annexb(data, sz, &payload);
		if (LIKELY(!ret)) {
			LOGV("annexbマーカーで始まってた");
			bool sps = false, pps = false;
			size_t ix = payload - data;
			sz -= ix;
			for (size_t i = ix; i < _actual_bytes; i++) {
				const auto type = (nal_unit_type_t)(payload[0] & 0x1fu);
				switch (type) {
				case NAL_UNIT_CODEC_SLICE_IDR: // これは来ないみたい
					LOGV("IFrameが見つかった");
					result = true;
					goto end;
				case NAL_UNIT_SEQUENCE_PARAM_SET:
				{
					LOGV("SPSが見つかった...次のannexbマーカーを探す");
					LOGV("ペイロード:%s", bin2hex(data, 128).c_str());
					LOGV("SPS:%s", bin2hex(payload, 128).c_str());
					result = sps = true;
					ret = find_annexb(&payload[1], sz - 1, &payload);
					if (LIKELY(!ret)) {
						i = payload - data;
						sz = _actual_bytes - i;
					} else {
						goto end;
					}
					break;
				}
				case NAL_UNIT_PICTURE_PARAM_SET:
				{
					LOGV("PPSが見つかった...次のannexbマーカーを探す");
					LOGV("ペイロード:%s", bin2hex(data, 128).c_str());
					LOGV("PPS:%s", bin2hex(payload, 128).c_str());
					result = pps = true;
					ret = find_annexb(&payload[1], sz  -1, &payload);
					if (LIKELY(!ret)) {
						i = payload - data;
						sz = _actual_bytes - i;
					} else {
						goto end;
					}
					break;
				}
				case NAL_UNIT_PICTURE_DELIMITER:
				{
					LOGV("IFrameじゃないけど1フレームを生成できるNALユニットの集まりの区切り");
					result = true;
					goto end;
				}
				default:
					// 何かAnnexbマーカーで始まるpayloadの時, SPS+PPSが見つかっていればIFrameとする
					LOGV("type=%x", type);
					goto end;
				} // end of switch (type)
			}
		} else {
			LOGD("annexbマーカーで始まってない");
		}
	} else {
		LOGD("h264フレームじゃない");
	}
end:
	RETURN(result, bool);
}

/**
 * annexBフォーマットの時に指定した位置からnalユニットを探す
 * 最初に見つかったnalユニットの先頭オフセットをoffsetに返す
 * annexBフォーマットでない時、見つからない時は負を返す
 * 見つかった時はnal_unit_type_tのいずれかを返す
 * @param in
 * @param offset
 * @return
 */
int VideoChecker::nal_unit_type(IVideoFrame &in, size_t &offset) {
	ENTER();

	int result = -1;
	const size_t sz = in.size();
	if (((RAW_FRAME_H264 == in.frame_type()) || (RAW_FRAME_FRAME_H264 == in.frame_type()))
		&& ((sz > 3) && (offset < sz - 4))) {
		const uint8_t *data = &in[offset];
		const uint8_t *payload = nullptr;
		if (!find_annexb(data, sz, &payload)) {
			result = (int)(payload[0] & 0x1fu);
			offset = (size_t)(payload - in.frame());
		}
	}

	RETURN(result, int);
}

} // namespace serenegiant::usb

/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_VIDEO_IMAGE_H
#define AANDUSB_VIDEO_IMAGE_H

#include <memory>
#include <vector>

#if defined(__ANDROID__)
// common
#include "hardware_buffer_stub.h"
#endif

#include "core/video.h"

namespace serenegiant::core {

#if defined(__ANDROID__)
// common
#define DEFAULT_LOCK_USAGE (AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN)
#else
#define DEFAULT_LOCK_USAGE (0)
#endif

class ImageBuffer;

/**
 * インターリーブ/プラナー/セミプラナー形式の映像情報を保持するためのホルダークラス
 */
typedef struct _image {
	/**
	 * フレームタイプ
	 */
	raw_frame_t frame_type;
	/**
	 * 映像幅[ピクセル数]
	 */
	uint32_t width;
	/**
	 * 映像高さ[ピクセル数]
	 */
	uint32_t height;
// プラナーの高さも入れるともう少しコードを集約できるかも?
//	union {
//		uint32_t height;
//		uint32_t height_y;
//	};
//	union {
//		struct {
//			uint32_t height_u;
//			uint32_t height_v;
//		};
//		struct {
//			uint32_t height_uv;
//		};
//	};
	/**
	 * 保持している映像データのバイト数
	 */
	size_t actual_bytes;
	/**
	 * 映像データを示すポインタ
	 * ptr_u = ptr_v = nullptr ならインターリーブ形式
	 * ptr_u - ptr_v == +1 ならy->vuのセミプラナー形式
	 * ptr_u - ptr_v == -1 ならy->uvのセミプラナー形式
	 * ptr_u - ptr_v > +1 ならy→v→uのプラナー形式
	 * ptr_u - ptr_v < -1 ならy→u→vのプラナー形式
	 */
	union {
		uint8_t *ptr;
		uint8_t *ptr_y;
	};
	uint8_t *ptr_u;
	uint8_t *ptr_v;
	// ポインターに対するオフセット値
	// インターリーブ形式:
	//   offset = y * stride + x * pixel_stride
	// yuvプラナー/セミプラナー形式:
	//   offset_y = y * stride_y + x * pixel_stride_y
	//   offset_u = y * stride_u + x * pixel_stride_u
	//   offset_v = y * stride_v + x * pixel_stride_v
	/**
	 * 映像のストライド(1行分の幅, ピクセル数)
	 */
	union {
		uint32_t stride;
		struct {
			uint32_t stride_y;
			uint32_t stride_u;
			uint32_t stride_v;
		};
	};
	/**
	 * 1ピクセル当たりのバイト数
	 * strideとpixel_strideが両方0ならフレームベースフォーマット(mjpegやh264等)
	 */
	union {
		uint32_t pixel_stride;		// インターリーブ形式
		struct {					// yuvプラナー/セミプラナー形式
			uint32_t pixel_stride_y;
			uint32_t pixel_stride_u;
			uint32_t pixel_stride_v;
		};
	};
} VideoImage_t;

typedef std::shared_ptr<VideoImage_t> VideoImageSp;
typedef std::unique_ptr<VideoImage_t> VideoImageUp;

/**
 * VideoImage_tの内容をログへ出力
 * @param image
 */
void dump(const VideoImage_t &image);

/**
 * フレームタイプがYUV planar/semi planarかどうかを取得
 * YUYVやUYVYはYUVだけどインターリーブ形式なのでfalseを返す
 * @param frame_type
 * @return
 */
bool is_yuv_format_type(const raw_frame_t &frame_type);

/**
 * 引数が同じフォーマットかどうかを確認
 * (ポインタが指しているメモリーが十分かどうかは確認できないので注意)
 * @param a
 * @param b
 * @param uv_check セミプラナー形式でuvとvuを区別するかどうか, デフォルトはtrueで区別する
 * @return
 */
int equal_format(const VideoImage_t &a, const VideoImage_t &b, const bool &uv_check = true);

/**
 * 映像データのポインターをVideoImage_tに割り当てる
 * @param image 映像データのポインターを割り当てるVideoImage_t
 * @param frame_type フレームタイプ
 * @param width 映像幅
 * @param height 映像高さ
 * @param data 映像データへのポインター, プラナー/セミプラナーの場合は各プレーンが連続していること
 * @param bytes 映像データのフレームサイズ
 * @return
 */
int assign(
	VideoImage_t &image,
	const raw_frame_t  &frame_type, const uint32_t &width, const uint32_t &height,
	uint8_t *data, const size_t &bytes);

#if defined(__ANDROID__)
/**
 * ハードウエアバッファをロックして映像データのポインターをVideoImage_tへ割り当てる
 * @param image
 * @param frame_type フレームタイプ
 * @param width 映像幅
 * @param height 映像高さ
 * @param graphicBuffer
 * @param lock_usage
 * @return
 */
int lock_and_assign(
	VideoImage_t &image,
	const raw_frame_t  &frame_type, const uint32_t &width, const uint32_t &height,
	AHardwareBuffer *graphicBuffer, const uint64_t &lock_usage);
#endif

/**
 * 単純コピー
 * @param src
 * @param dst
 * @param work
 * @param uv_check セミプラナー形式でuvとvuを区別するかどうか, デフォルトはtrueで区別する
 * @return
 */
int copy(
	const VideoImage_t &src, VideoImage_t &dst,
	std::vector<uint8_t> &work,
	const bool &uv_check = true);

/**
 * YUV系の映像を別のフレームタイプに変換する
 * @param src
 * @param dst
 * @param work
 * @return
 */
int yuv_copy(
	const VideoImage_t &src, VideoImage_t &dst,
	std::vector<uint8_t> &work);

/**
 * 単純コピー
 * @param src
 * @param dst
 * @param work
 * @param uv_check セミプラナー形式でuvとvuを区別するかどうか, デフォルトはtrueで区別する
 * @return
 */
int copy(
	const ImageBuffer &src, ImageBuffer &dst,
	std::vector<uint8_t> &work,
	const bool &uv_check = true);

}	// namespace serenegiant::usb

#endif //AANDUSB_VIDEO_IMAGE_H

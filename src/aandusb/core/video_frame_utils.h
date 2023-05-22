/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef AANDUSB_UVC_FRAME_UTILS_H
#define AANDUSB_UVC_FRAME_UTILS_H

#include <turbojpeg.h>
// core
#include "core/video.h"

namespace serenegiant::core {

class IVideoFrame;

typedef enum nal_unit_type {
	NAL_UNIT_UNSPECIFIED = 0,
	NAL_UNIT_CODEC_SLICE = 1,			// Coded slice of a non-IDR picture == PFrame for AVC
	NAL_UNIT_CODEC_SLICE_A = 2,			// Coded slice data partition A
	NAL_UNIT_CODEC_SLICE_B = 3,			// Coded slice data partition B
	NAL_UNIT_CODEC_SLICE_C = 4,			// Coded slice data partition C
	NAL_UNIT_CODEC_SLICE_IDR = 5,		// Coded slice of an IDR picture == IFrame for AVC
	NAL_UNIT_SEI = 6,					// supplemental enhancement information
	NAL_UNIT_SEQUENCE_PARAM_SET = 7,	// Sequence parameter set == SPS for AVC
	NAL_UNIT_PICTURE_PARAM_SET = 8,		// Picture parameter set == PPS for AVC
	NAL_UNIT_PICTURE_DELIMITER = 9,		// access unit delimiter (AUD)
	NAL_UNIT_END_OF_SEQUENCE = 10,		// End of sequence
	NAL_UNIT_END_OF_STREAM = 11,		// End of stream
	NAL_UNIT_FILLER = 12,				// Filler data
	NAL_UNIT_RESERVED_13 = 13,			// Sequence parameter set extension
	NAL_UNIT_RESERVED_14 = 14,			// Prefix NAL unit
	NAL_UNIT_RESERVED_15 = 15,			// Subset sequence parameter set
	NAL_UNIT_RESERVED_16 = 16,
	NAL_UNIT_RESERVED_17 = 17,
	NAL_UNIT_RESERVED_18 = 18,
	NAL_UNIT_RESERVED_19 = 19,			// Coded slice of an auxiliary coded picture without partitioning
	NAL_UNIT_RESERVED_20 = 20,			// Coded slice extension
	NAL_UNIT_RESERVED_21 = 21,			// Coded slice extension for depth view components
	NAL_UNIT_RESERVED_22 = 22,
	NAL_UNIT_RESERVED_23 = 23,
	NAL_UNIT_UNSPECIFIED_24 = 24,
	NAL_UNIT_UNSPECIFIED_25 = 25,
	NAL_UNIT_UNSPECIFIED_26 = 26,
	NAL_UNIT_UNSPECIFIED_27 = 27,
	NAL_UNIT_UNSPECIFIED_28 = 28,
	NAL_UNIT_UNSPECIFIED_29 = 29,
	NAL_UNIT_UNSPECIFIED_30 = 30,
	NAL_UNIT_UNSPECIFIED_31 = 31,
} nal_unit_type_t;

#define JFIF_MARKER_MSB 0xff
// 上位バイトは省略(0xff)
typedef enum jfif_marker {
	JFIF_MARKER_FF		= 0x00,	// 0xff00 == 0xffそのもの
	JFIF_MARKER_TEM		= 0x01,	// 算術符号用テンポラリ
//	JFIF_MARKER_RES		= 0x02,	// リザーブ
//	// この間は全部リザーブ
//	JFIF_MARKER_RES		= 0xBF,	// リザーブ
//	JFIF_MARKER_RES		= 0x4e,	// リザーブ
	JFIF_MARKER_SOC		= 0x4F,	// コードストリーム開始(JPEG2000)
	JFIF_MARKER_SIZ		= 0x51,	// サイズ定義(JPEG2000)
	JFIF_MARKER_COD		= 0x52,	// 符号化標準定義(JPEG2000)
	JFIF_MARKER_COC		= 0x53,	// 符号化個別定義(JPEG2000)
	JFIF_MARKER_TLM		= 0x55,	// タイルパート長定義(JPEG2000)
	JFIF_MARKER_PLM		= 0x57,	// パケット長標準定義(JPEG2000)
	JFIF_MARKER_PLT		= 0x58,	// パケット長個別定義(JPEG2000)
	JFIF_MARKER_QCD		= 0x5C,	// 量子化標準定義(JPEG2000)
	JFIF_MARKER_QCC		= 0x5D,	// 量子化個別定義(JPEG2000)
	JFIF_MARKER_RGN		= 0x5E,	// ROI定義(JPEG2000)
	JFIF_MARKER_POC		= 0x5F,	// プログレッション順序変更(JPEG2000)
	JFIF_MARKER_PPM		= 0x60,	// パケットヘッダー標準定義(JPEG2000)
	JFIF_MARKER_PPT		= 0x61,	// パケットヘッダー個別定義(JPEG2000)
	JFIF_MARKER_CRG		= 0x63,	// コンポーネント位相定義(JPEG2000)
	JFIF_MARKER_COM2	= 0x64,	// コメント(JPEG2000)
	JFIF_MARKER_SOT		= 0x90,	// タイルパート開始(JPEG2000)
	JFIF_MARKER_SOP		= 0x91,	// パケット開始(JPEG2000)
	JFIF_MARKER_EPH		= 0x92,	// パケットヘッダー終了(JPEG2000)
	JFIF_MARKER_SOD		= 0x93,	// データ開始(JPEG2000)
	JFIF_MARKER_SOF0	= 0xC0,	// ハフマン式コードのベースラインDCTフレーム	, フレームタイプ０開始セグメント
	JFIF_MARKER_SOF1	= 0xC1,	// ハフマン式コードの拡張シーケンシャルDCTフレーム, フレームタイプ１開始セグメント
	JFIF_MARKER_SOF2	= 0xC2,	// ハフマン式コードのプログレッシブDCTフレーム, フレームタイプ２開始セグメント
	JFIF_MARKER_SOF3	= 0xC3,	// ハフマン式コードの可逆(Spatial DPCM)フレーム	, フレームタイプ３開始セグメント
	JFIF_MARKER_DHT		= 0xC4,	// ハフマン法テーブル定義セグメント
	JFIF_MARKER_SOF5	= 0xC5,	// ハフマン式コードの差分拡張シーケンシャルフレーム, フレームタイプ５開始セグメント
	JFIF_MARKER_SOF6	= 0xC6,	// ハフマン式コードの差分プログレッシブフレーム, フレームタイプ６開始セグメント
	JFIF_MARKER_SOF7	= 0xC7,	// ハフマン式コードの差分可逆(Spatial DPCM)フレーム, フレームタイプ７開始セグメント
	JFIF_MARKER_JPG		= 0xC8,	// 拡張のための予備
	JFIF_MARKER_SOF9	= 0xC9,	// 算術式コードの拡張シーケンシャルDCTフレーム, フレームタイプ９開始セグメント
	JFIF_MARKER_SOF10	= 0xCA,	// 算術式コードのプログレッシブDCTフレーム, フレームタイプ１０開始セグメント
	JFIF_MARKER_SOF11	= 0xCB,	// 算術式コードの可逆(Spatial DPCM)フレーム, フレームタイプ１１開始セグメント
	JFIF_MARKER_DAC		= 0xCC,	// 算術式圧縮テーブルを定義する
	JFIF_MARKER_SOF13	= 0xCD,	// 算術式コードの差分拡張シーケンシャルDCTフレーム, フレームタイプ１３開始セグメント
	JFIF_MARKER_SOF14	= 0xCE,	// 算術式コードの差分プログレッシブDCTフレーム, フレームタイプ１４開始セグメント
	JFIF_MARKER_SOF15	= 0xCF,	// 算術式コードの差分の可逆(Spatial DPCM)フレーム, フレームタイプ１５開始セグメント
	JFIF_MARKER_RST0	= 0xd0,	// リスタートマーカー0, data 無し
	JFIF_MARKER_RST1	= 0xd1,	// リスタートマーカー1, data 無し
	JFIF_MARKER_RST2	= 0xd2, // リスタートマーカー2, data 無し
	JFIF_MARKER_RST3	= 0xd3, // リスタートマーカー3, data 無し
	JFIF_MARKER_RST4	= 0xd4, // リスタートマーカー4, data 無し
	JFIF_MARKER_RST5	= 0xd5, // リスタートマーカー5, data 無し
	JFIF_MARKER_RST6	= 0xd6,	// リスタートマーカー6, data 無し
	JFIF_MARKER_RST7	= 0xd7, // リスタートマーカー7, data 無し
	JFIF_MARKER_SOI		= 0xd8,	// SOI, data 無し
	JFIF_MARKER_EOI		= 0xd9,	// EOI, data 無し
	JFIF_MARKER_SOS		= 0xDA,	// , スキャンの開始セグメント
	JFIF_MARKER_DQT		= 0xDB,	// , 量子化テーブル定義セグメント
	JFIF_MARKER_DNL		= 0xDC,	// 行数を定義する
	JFIF_MARKER_DRI		= 0xDD,	// リスタートの間隔を定義する
	JFIF_MARKER_DHP		= 0xDE,	// 階層行数を定義する(ハイアラーキカル方式)
	JFIF_MARKER_EXP		= 0xDF,	// 標準イメージの拡張参照
	JFIF_MARKER_APP0	= 0xE0,	// JFIF(JPEGはこれ！), タイプ０のアプリケーションセグメント
	JFIF_MARKER_APP1	= 0xE1,	// Exif、http:, タイプ１のアプリケーションセグメント
	JFIF_MARKER_APP2	= 0xE2,	// ICC_P, タイプ２のアプリケーションセグメント
	JFIF_MARKER_APP3	= 0xE3,	// , タイプ３のアプリケーションセグメント
	JFIF_MARKER_APP4	= 0xE4,	// , タイプ４のアプリケーションセグメント
	JFIF_MARKER_APP5	= 0xE5,	// , タイプ５のアプリケーションセグメント
	JFIF_MARKER_APP6	= 0xE6,	// , タイプ６のアプリケーションセグメント
	JFIF_MARKER_APP7	= 0xE7,	// , タイプ７のアプリケーションセグメント
	JFIF_MARKER_APP8	= 0xE8,	// , タイプ８のアプリケーションセグメント
	JFIF_MARKER_APP9	= 0xE9,	// , タイプ９のアプリケーションセグメント
	JFIF_MARKER_APP10	= 0xEA,	// , タイプ１０のアプリケーションセグメント
	JFIF_MARKER_APP11	= 0xEB,	// , タイプ１１のアプリケーションセグメント
	JFIF_MARKER_APP12	= 0xEC,	// Ducky, タイプ１２のアプリケーションセグメント
	JFIF_MARKER_APP13	= 0xED,	// Photo, タイプ１３のアプリケーションセグメント
	JFIF_MARKER_APP14	= 0xEE,	// Adobe, タイプ１４のアプリケーションセグメント
	JFIF_MARKER_APP15	= 0xEF,	// , タイプ１５のアプリケーションセグメント
	JFIF_MARKER_JPG0	= 0xF0,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG1	= 0xF1,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG2	= 0xF2,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG3	= 0xF3,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG4	= 0xF4,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG5	= 0xF5,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG6	= 0xF6,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG7	= 0xF7,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG8	= 0xF8,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG9	= 0xF9,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG10	= 0xFA,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG11	= 0xFB,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG12	= 0xFC,	// JPEGの拡張のための予備
	JFIF_MARKER_JPG13	= 0xFD,	// JPEGの拡張のための予備
	JFIF_MARKER_COM		= 0xFE,	// コメントセグメント
} jfif_marker_t;

/**
 * FOURCCからraw_frame_tを取得する。見つからなければRAW_FRAME_UNKNOWN
 * @param fourcc
 * @return
 */
raw_frame_t get_raw_frame_type_from_fourcc(const uint8_t *fourcc);
/**
 * guidからraw_frame_tを取得する。見つからなければRAW_FRAME_UNKNOWN
 * @param guid
 * @return
 */
raw_frame_t get_raw_frame_type_from_guid(const uint8_t *guid);

/**
 * JPEGデータから指定したAPPマーカーを探してマーカー直後のデータへのポインタを返す
 * 見つからなければnullptrを返す
 * @param marker
 * @param data
 * @param size
 * @param len
 * @return
 */
const uint8_t *find_app_marker(const uint8_t &marker,
	const uint8_t *data, const size_t &size, size_t &len);

/**
 * JFIFマーカーを手繰って(m)jpegフォーマットが正しいかどうかを確認する
 * @param frame
 * @param data
 * @param size
 * @return
 */
int sanitaryMJPEG(IVideoFrame &frame, const uint8_t *data, const size_t &size);

/**
 * libjpeg-turboのJ_COLOR_SPACEに対応するraw_frame_tを取得
 * 対応するものがなければRAW_FRAME_UNKNOWNを返す
 * @param color_space
 * @return
 */
raw_frame_t tj_color_space2raw_frame(const int/*J_COLOR_SPACE*/ &color_space);

/**
 * libjpeg-turboのピクセルフォーマットからraw_frame_tへ変換
 * 対応するものがなければRAW_FRAME_UNKNOWNを返す
 * @param tj_pixel_format
 * @return
 */
raw_frame_t tj_pixel_format2raw_frame(const int &tj_pixel_format);

/**
 * turbo-jpegのサブサンプリングからraw_frame_tを取得
 * @param jpeg_subsamp
 * @return
 */
raw_frame_t tjsamp2raw_frame(const int &jpeg_subsamp);

/**
 * (m)jpegのヘッダーを解析してサブサンプリングや解像度情報を取得する
 * tjDecompressHeader2呼び出しのヘルパー関数
 * @param jpegDecompressor
 * @param in
 * @param jpeg_bytes
 * @param jpeg_subsamp
 * @param jpeg_width
 * @param jpeg_height
 * @return
 */
int parse_mjpeg_header(
	tjhandle &jpegDecompressor,
	const IVideoFrame &in,
	size_t &jpeg_bytes, int &jpeg_subsamp,
	int &jpeg_width, int &jpeg_height);

raw_frame_t get_mjpeg_decode_type(
	tjhandle &jpegDecompressor,
	const IVideoFrame &src);

raw_frame_t checkFOURCC(const uint8_t *data, const size_t &size);

}	// namespace serenegiant::usb

#endif //AANDUSB_UVC_FRAME_UTILS_H

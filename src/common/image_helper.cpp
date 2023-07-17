/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#if 1    // set 0 if you need debug log, otherwise set 1
	#ifndef LOG_NDEBUG
	#define LOG_NDEBUG
	#endif
	#undef USE_LOGALL
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include <cstring>  // memcpy
#include <png.h>

#include "utilbase.h"
#include "image_helper.h"

namespace serenegiant::media {


#define SIGNATURE_NUM (8)

Image::Image(const size_t &bytes) {
    ENTER();

    resize(bytes);

    EXIT();
}

Image::~Image() noexcept {
    ENTER();

    EXIT();
}

size_t Image::resize(const size_t &bytes) {
    ENTER();

    if (bytes != size()) {
        _data.resize(bytes);
    }

    RETURN(size(), size_t);
}

//--------------------------------------------------------------------------------
int read_png_from_file(Image &bitmap, const char *filename) {

    png_byte signature[SIGNATURE_NUM];

    FILE *fi = fopen(filename, "rb");
    if (!fi) {
        LOGE("failed to opne %s", filename);
        return -1;
    }

    const auto readSize = fread(signature, 1, SIGNATURE_NUM, fi);

    if (png_sig_cmp(signature, 0, SIGNATURE_NUM)) {
        LOGE("png_sig_cmp error!");
        fclose(fi);
        return -1;
    }

    auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        LOGE("png_create_read_struct error!");
        fclose(fi);
        return -1;
    }

    auto info = png_create_info_struct(png);
    if (!info) {
        LOGE("png_crete_info_struct error!");
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fi);
        return -1;
    }

    png_init_io(png, fi);
    png_set_sig_bytes(png, readSize);
    png_read_png(png, info, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16, nullptr);

    const auto width = png_get_image_width(png, info);
    const auto height = png_get_image_height(png, info);

    auto datap = png_get_rows(png, info);

    const auto type = png_get_color_type(png, info);
    // テクスチャがRGBAで変換するのは面倒なのでRGBAのみに対応する
    if (type != PNG_COLOR_TYPE_RGB_ALPHA) {
        LOGE("color type is not RGB or RGBA,type=%d", type);
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fi);
        return -1;
    }

    bitmap.width = width;
    bitmap.height = height;
    bitmap.color_channel = 4;
    LOGD("width=%d,height=%d,ch=%d", bitmap.width, bitmap.height, bitmap.color_channel);

    bitmap.resize(bitmap.width * bitmap.height * bitmap.color_channel);
    if (bitmap.data() == nullptr) {
        LOGE("failed to allocate data memory");
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fi);
        return -1;
    }

    const auto stride = width * bitmap.color_channel;
    for (uint32_t j = 0; j < height; j++) {
        memcpy(&bitmap[j * stride], datap[j], stride);
    }

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fi);

    return 0;
}

}   // namespace serenegiant::media

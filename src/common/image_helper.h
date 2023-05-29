/*
 * aAndUsb
 * Copyright (c) 2014-2023 saki t_saki@serenegiant.com
 * Distributed under the terms of the GNU Lesser General Public License (LGPL v3.0) License.
 * License details are in the file license.txt, distributed as part of this software.
 */

#ifndef IMAGE_HELPER_H_
#define IMAGE_HELPER_H_

#include <cstdlib>
#include <stdint.h>
#include <vector>
#include <png.h>

namespace serenegiant::media {

class Image {
private:
    std::vector<uint8_t> _data;
public:
    uint32_t height;
    uint32_t width;
    uint32_t color_channel;

    Image(const size_t &bytes = 0);
    ~Image() noexcept;
    size_t resize(const size_t &bytes);

    inline size_t size() const { return _data.size(); };
	inline uint8_t *data() { return &_data[0]; };
	inline const uint8_t *data() const { return &_data[0]; };
	inline uint8_t &operator[](size_t ix) { return _data[ix]; };
	inline const uint8_t &operator[](size_t ix) const { return _data[ix]; };
};

/**
 * @brief PNG画像をファイルからImageへ読み込む
 * 
 * @param bitmap 
 * @param filename 
 * @return int 
 */
int read_png_from_file(Image &bitmap, const char *filename);

}   // namespace serenegiant::media

#endif // IMAGE_HELPER_H_

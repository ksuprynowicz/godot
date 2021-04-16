/*************************************************************************/
/*  image_compress_cvtt.cpp                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "image_compress_cvtt.h"

#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/string/print_string.h"
#include "core/templates/safe_refcount.h"

#include <ConvectionKernels.h>

void image_decompress_cvtt(Image *p_image) {
	Image::Format target_format;
	bool is_signed = false;
	bool is_hdr = false;

	Image::Format input_format = p_image->get_format();

	switch (input_format) {
		case Image::FORMAT_BPTC_RGBA:
			target_format = Image::FORMAT_RGBA8;
			break;
		case Image::FORMAT_BPTC_RGBF:
		case Image::FORMAT_BPTC_RGBFU:
			target_format = Image::FORMAT_RGBH;
			is_signed = (input_format == Image::FORMAT_BPTC_RGBF);
			is_hdr = true;
			break;
		default:
			return; // Invalid input format
	};

	int w = p_image->get_width();
	int h = p_image->get_height();

	const uint8_t *rb = p_image->get_data().ptr();

	Vector<uint8_t> data;
	int target_size = Image::get_image_data_size(w, h, target_format, p_image->has_mipmaps());
	int mm_count = p_image->get_mipmap_count();
	data.resize(target_size);

	uint8_t *wb = data.ptrw();

	int bytes_per_pixel = is_hdr ? 6 : 4;

	int dst_ofs = 0;

	for (int i = 0; i <= mm_count; i++) {
		int src_ofs = p_image->get_mipmap_offset(i);

		const uint8_t *in_bytes = &rb[src_ofs];
		uint8_t *out_bytes = &wb[dst_ofs];

		cvtt::PixelBlockU8 output_blocks_ldr[cvtt::NumParallelBlocks];
		cvtt::PixelBlockF16 output_blocks_hdr[cvtt::NumParallelBlocks];

		for (int y_start = 0; y_start < h; y_start += 4) {
			int y_end = y_start + 4;

			for (int x_start = 0; x_start < w; x_start += 4 * cvtt::NumParallelBlocks) {
				int x_end = x_start + 4 * cvtt::NumParallelBlocks;

				uint8_t input_blocks[16 * cvtt::NumParallelBlocks];
				memset(input_blocks, 0, sizeof(input_blocks));

				unsigned int num_real_blocks = ((w - x_start) + 3) / 4;
				if (num_real_blocks > cvtt::NumParallelBlocks) {
					num_real_blocks = cvtt::NumParallelBlocks;
				}

				memcpy(input_blocks, in_bytes, 16 * num_real_blocks);
				in_bytes += 16 * num_real_blocks;

				if (is_hdr) {
					if (is_signed) {
						cvtt::Kernels::DecodeBC6HS(output_blocks_hdr, input_blocks);
					} else {
						cvtt::Kernels::DecodeBC6HU(output_blocks_hdr, input_blocks);
					}
				} else {
					cvtt::Kernels::DecodeBC7(output_blocks_ldr, input_blocks);
				}

				for (int y = y_start; y < y_end; y++) {
					int first_input_element = (y - y_start) * 4;
					uint8_t *row_start;
					if (y >= h) {
						row_start = out_bytes + (h - 1) * (w * bytes_per_pixel);
					} else {
						row_start = out_bytes + y * (w * bytes_per_pixel);
					}

					for (int x = x_start; x < x_end; x++) {
						uint8_t *pixel_start;
						if (x >= w) {
							pixel_start = row_start + (w - 1) * bytes_per_pixel;
						} else {
							pixel_start = row_start + x * bytes_per_pixel;
						}

						int block_index = (x - x_start) / 4;
						int block_element = (x - x_start) % 4 + first_input_element;
						if (is_hdr) {
							memcpy(pixel_start, output_blocks_hdr[block_index].m_pixels[block_element], bytes_per_pixel);
						} else {
							memcpy(pixel_start, output_blocks_ldr[block_index].m_pixels[block_element], bytes_per_pixel);
						}
					}
				}
			}
		}

		dst_ofs += w * h * bytes_per_pixel;
		w >>= 1;
		h >>= 1;
	}

	p_image->create(p_image->get_width(), p_image->get_height(), p_image->has_mipmaps(), target_format, data);
}

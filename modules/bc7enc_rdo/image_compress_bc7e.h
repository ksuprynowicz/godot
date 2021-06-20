/*************************************************************************/
/*  image_compress_bc7e.h                                                */
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

#ifndef IMAGE_COMPRESS_BC7E_H
#define IMAGE_COMPRESS_BC7E_H

#include "bc7enc_rdo.h"

#include "core/error/error_macros.h"
#include "thirdparty/bc7e/rdo_bc_encoder.h"

#include "core/io/image.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/os/threaded_array_processor.h"
#include "core/string/print_string.h"
#include <stdint.h>

void image_compress_bc7e(Image *p_image, float p_lossy_quality, Image::UsedChannels p_channels) {
	Image::Format input_format = p_image->get_format();
	if (p_image->is_compressed()) {
		return; //do not compress, already compressed
	}
	if (input_format != Image::FORMAT_RGB8 && input_format != Image::FORMAT_RGBA8) {
		return;
	}

	uint32_t start_t = OS::get_singleton()->get_ticks_msec();
	Image::Format target_format = Image::FORMAT_BPTC_RGBA;

	rdo_bc::rdo_bc_encoder encoder;
	int uber_level = 4;
	if (Math::is_equal_approx(p_lossy_quality, 1.0f)) {
		uber_level = 4;
	} else if (p_lossy_quality > 0.85) {
		uber_level = 4;
	} else if (p_lossy_quality > 0.75) {
		uber_level = 3;
	} else if (p_lossy_quality > 0.55) {
		uber_level = 3;
	} else if (p_lossy_quality > 0.35) {
		uber_level = 3;
	} else if (p_lossy_quality > 0.15) {
		uber_level = 2;
	}

	rdo_bc::rdo_bc_params pack_params;
	switch (uber_level) {
		case 0:
			pack_params.m_bc7_uber_level = 0;
			break;
		case 1:
			pack_params.m_bc7_uber_level = 1;
			break;
		case 2:
			pack_params.m_bc7_uber_level = 2;
			break;
		case 3:
			pack_params.m_bc7_uber_level = 3;
			break;
		case 4:
			pack_params.m_bc7_uber_level = 4;
			break;
		case 5:
			pack_params.m_bc7_uber_level = 5;
			break;
		case 6:
		default:
			pack_params.m_bc7_uber_level = 6;
			break;
	}
	Ref<Image> new_img;
	new_img.instantiate();
	new_img->create(p_image->get_width(), p_image->get_height(), p_image->has_mipmaps(), target_format);

	Vector<uint8_t> data = new_img->get_data();

	const bool mipmaps = new_img->has_mipmaps();
	const int width = new_img->get_width();
	const int height = new_img->get_height();

	uint8_t *wr = data.ptrw();

	Ref<Image> image = p_image->duplicate();
	int mip_count = mipmaps ? Image::get_image_required_mipmaps(width, height, target_format) : 0;
	for (int i = 0; i < mip_count + 1; i++) {
		int ofs, size, mip_w, mip_h;
		new_img->get_mipmap_offset_size_and_dimensions(i, ofs, size, mip_w, mip_h);
		mip_w = (mip_w + 3) & ~3;
		mip_h = (mip_h + 3) & ~3;
		image->resize(mip_w, mip_h);
		utils::image_u8 mip_source_image(mip_w, mip_h);
		PackedByteArray image_data = image->get_data();
		for (int32_t y = 0; y < mip_h; y++) {
			for (int32_t x = 0; x < mip_w; x++) {
				Color c = image->get_pixel(x, y);
				uint8_t r = c.get_r8();
				uint8_t g = c.get_g8();
				uint8_t b = c.get_b8();
				uint8_t a = c.get_a8();
				mip_source_image(x, y).set(r, g, b, a);
			}
		}
		ERR_FAIL_COND_MSG(!encoder.init(mip_source_image, pack_params), "bc7enc_rdo did not begin.");
		ERR_FAIL_COND_MSG(!encoder.encode(), "bc7enc_rdo could not encode.");
		Vector<uint8_t> packed_image;
		packed_image.resize(encoder.get_total_blocks_size_in_bytes());

		int target_size = packed_image.size();
		ERR_FAIL_COND(target_size != size);
		memcpy(&wr[ofs], encoder.get_blocks(), size);
	}

	Dictionary rgba_metrics = p_image->compute_image_metrics(new_img, false);
	print_line(vformat("RGBA\tMax error: %.0f RMSE: %.2f PSNR %.2f dB", rgba_metrics["max"], rgba_metrics["root_mean_squared"], rgba_metrics["peak_snr"]));

	p_image->create(new_img->get_width(), new_img->get_height(), new_img->has_mipmaps(), new_img->get_format(), data);
	uint32_t end_t = OS::get_singleton()->get_ticks_msec();
	print_verbose(vformat("bc7enc_rdo %dx%d total time: %.2f secs", new_img->get_width(), new_img->get_height(), (double)(end_t - start_t) / 1000.f));
}

#endif // IMAGE_COMPRESS_BC7E_H

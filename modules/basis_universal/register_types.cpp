/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "register_types.h"

#include "core/os/os.h"
#include "servers/rendering_server.h"
#include <encoder/basisu_frontend.h>

#ifdef TOOLS_ENABLED
#include <encoder/basisu_comp.h>
#include <encoder/basisu_enc.h>
#endif

#include <transcoder/basisu_transcoder.h>

enum BasisDecompressFormat {
	BASIS_DECOMPRESS_RG,
	BASIS_DECOMPRESS_RGB,
	BASIS_DECOMPRESS_RGBA,
	BASIS_DECOMPRESS_RG_AS_RA
};

//workaround for lack of ETC2 RG
#define USE_RG_AS_RGBA

#ifdef TOOLS_ENABLED
static Vector<uint8_t> basis_universal_packer(const Ref<Image> &p_image, Image::UsedChannels p_channels) {
	Vector<uint8_t> budata;
	{
		basisu::basis_compressor_params params;
		Ref<Image> image = p_image->duplicate();
		if (image->get_format() != Image::FORMAT_RGBA8) {
			image->convert(Image::FORMAT_RGBA8);
		}
		Ref<Image> image_single = image->duplicate();
		{
			if (image_single->has_mipmaps()) {
				image_single->clear_mipmaps();
			}
			basisu::image buimg(image_single->get_width(), image_single->get_height());
			Vector<uint8_t> vec = image_single->get_data();
			const uint8_t *r = vec.ptr();
			memcpy(buimg.get_ptr(), r, vec.size());
			params.m_source_images.push_back(buimg);
		}
		basisu::vector<basisu::image> source_images;
		for (int32_t mipmap_i = 1; mipmap_i < image->get_mipmap_count(); mipmap_i++) {
			Ref<Image> mip = image->get_image_from_mipmap(mipmap_i);
			basisu::image buimg(mip->get_width(), mip->get_height());
			Vector<uint8_t> vec = mip->get_data();
			const uint8_t *r = vec.ptr();
			memcpy(buimg.get_ptr(), r, vec.size());
			source_images.push_back(buimg);
		}
		params.m_source_mipmap_images.push_back(source_images);
		params.m_quality_level = basisu::BASISU_QUALITY_MIN;
		params.m_mip_fast = false;
		params.m_multithreading = true;
		params.m_uastc = true;
		params.m_rdo_uastc = true;
		params.m_rdo_uastc_multithreading = true;

		basisu::job_pool jpool(OS::get_singleton()->get_processor_count());
		params.m_pJob_pool = &jpool;

		BasisDecompressFormat decompress_format = BASIS_DECOMPRESS_RG;
		params.m_check_for_alpha = false;

		switch (p_channels) {
			case Image::USED_CHANNELS_L: {
				decompress_format = BASIS_DECOMPRESS_RGB;
			} break;
			case Image::USED_CHANNELS_LA: {
				params.m_force_alpha = true;
				decompress_format = BASIS_DECOMPRESS_RGBA;
			} break;
			case Image::USED_CHANNELS_R: {
				decompress_format = BASIS_DECOMPRESS_RGB;
			} break;
			case Image::USED_CHANNELS_RG: {
#ifdef USE_RG_AS_RGBA
				image->convert_rg_to_ra_rgba8();
				decompress_format = BASIS_DECOMPRESS_RG_AS_RA;
#else
				params.m_seperate_rg_to_color_alpha = true;
				decompress_format = BASIS_DECOMPRESS_RG;
#endif
			} break;
			case Image::USED_CHANNELS_RGB: {
				decompress_format = BASIS_DECOMPRESS_RGB;
			} break;
			case Image::USED_CHANNELS_RGBA: {
				params.m_force_alpha = true;
				decompress_format = BASIS_DECOMPRESS_RGBA;
			} break;
		}

		basisu::basis_compressor c;
		c.init(params);

		int buerr = c.process();
		ERR_FAIL_COND_V(buerr != basisu::basis_compressor::cECSuccess, budata);

		const basisu::uint8_vec &buvec = c.get_output_basis_file();
		budata.resize(buvec.size() + 4);

		{
			uint8_t *w = budata.ptrw();
			uint32_t *decf = (uint32_t *)w;
			*decf = decompress_format;
			memcpy(w + 4, &buvec[0], buvec.size());
		}
	}

	return budata;
}
#endif // TOOLS_ENABLED

static Ref<Image> basis_universal_unpacker_ptr(const uint8_t *p_data, int p_size) {
	Ref<Image> image;

	const uint8_t *ptr = p_data;
	int size = p_size;

	basist::transcoder_texture_format format = basist::transcoder_texture_format::cTFTotalTextureFormats;
	Image::Format imgfmt = Image::FORMAT_MAX;

	switch (*(uint32_t *)(ptr)) {
		case BASIS_DECOMPRESS_RG: {
			if (RS::get_singleton()->has_os_feature("rgtc")) {
				format = basist::transcoder_texture_format::cTFBC5; // get this from renderer
				imgfmt = Image::FORMAT_RGTC_RG;
			} else if (RS::get_singleton()->has_os_feature("etc2")) {
				//unfortunately, basis universal does not support
				//
				ERR_FAIL_V(image); //unimplemented here
				//format = basist::transcoder_texture_format::cTFETC1; // get this from renderer
				//imgfmt = Image::FORMAT_RGTC_RG;
			} else {
				// FIXME: There wasn't anything here, but then imgformat is used uninitialized.
				ERR_FAIL_V(image);
			}
		} break;
		case BASIS_DECOMPRESS_RGB: {
			if (RS::get_singleton()->has_os_feature("bptc")) {
				format = basist::transcoder_texture_format::cTFBC7_M6_OPAQUE_ONLY; // get this from renderer
				imgfmt = Image::FORMAT_BPTC_RGBA;
			} else if (RS::get_singleton()->has_os_feature("s3tc")) {
				format = basist::transcoder_texture_format::cTFBC1; // get this from renderer
				imgfmt = Image::FORMAT_DXT1;
			} else if (RS::get_singleton()->has_os_feature("etc")) {
				format = basist::transcoder_texture_format::cTFETC1; // get this from renderer
				imgfmt = Image::FORMAT_ETC;
			} else {
				format = basist::transcoder_texture_format::cTFBGR565; // get this from renderer
				imgfmt = Image::FORMAT_RGB565;
			}

		} break;
		case BASIS_DECOMPRESS_RGBA: {
			if (RS::get_singleton()->has_os_feature("bptc")) {
				format = basist::transcoder_texture_format::cTFBC7_M5; // get this from renderer
				imgfmt = Image::FORMAT_BPTC_RGBA;
			} else if (RS::get_singleton()->has_os_feature("s3tc")) {
				format = basist::transcoder_texture_format::cTFBC3; // get this from renderer
				imgfmt = Image::FORMAT_DXT5;
			} else if (RS::get_singleton()->has_os_feature("etc2")) {
				format = basist::transcoder_texture_format::cTFETC2; // get this from renderer
				imgfmt = Image::FORMAT_ETC2_RGBA8;
			} else {
				//opengl most likely
				format = basist::transcoder_texture_format::cTFRGBA4444; // get this from renderer
				imgfmt = Image::FORMAT_RGBA4444;
			}
		} break;
		case BASIS_DECOMPRESS_RG_AS_RA: {
			if (RS::get_singleton()->has_os_feature("s3tc")) {
				format = basist::transcoder_texture_format::cTFBC3; // get this from renderer
				imgfmt = Image::FORMAT_DXT5_RA_AS_RG;
			} else if (RS::get_singleton()->has_os_feature("etc2")) {
				format = basist::transcoder_texture_format::cTFETC2; // get this from renderer
				imgfmt = Image::FORMAT_ETC2_RGBA8;
			} else {
				//opengl most likely, bad for normal maps, nothing to do about this.
				format = basist::transcoder_texture_format::cTFRGBA32;
				imgfmt = Image::FORMAT_RGBA8;
			}
		} break;
	}

	ptr += 4;
	size -= 4;

	basist::basisu_transcoder tr;

	ERR_FAIL_COND_V(!tr.validate_header(ptr, size), image);

	basist::basisu_file_info info;
	tr.get_file_info(ptr, size, info);
	basist::basisu_image_info image_info;
	tr.get_image_info(ptr, size, image_info, 0);

	int block_size = basist::basis_get_bytes_per_block_or_pixel(format);
	Vector<uint8_t> gpudata;
	ERR_FAIL_INDEX_V(0, info.m_image_mipmap_levels.size(), Ref<Image>());
	uint32_t total_mip_levels = info.m_image_mipmap_levels[0];
	gpudata.resize(Image::get_image_data_size(image_info.m_width, image_info.m_height, imgfmt, total_mip_levels > 1));

	{
		uint8_t *w = gpudata.ptrw();
		uint8_t *dst = w;
		for (int i = 0; i < gpudata.size(); i++) {
			dst[i] = 0x00;
		}

		int ofs = 0;
		tr.start_transcoding(ptr, size);
		for (uint32_t i = 0; i < total_mip_levels; i++) {
			basist::basisu_image_level_info level;
			tr.get_image_level_info(ptr, size, level, 0, i);

			bool ret = tr.transcode_image_level(ptr, size, 0, i, dst + ofs, level.m_total_blocks, format);
			if (!ret) {
				printf("failed! on level %u\n", i);
				break;
			};

			ofs += level.m_total_blocks * block_size;
		};
		image.instantiate();
		image->create(image_info.m_width, image_info.m_height, total_mip_levels > 1, imgfmt, gpudata);
	}

	return image;
}

static Ref<Image> basis_universal_unpacker(const Vector<uint8_t> &p_buffer) {
	Ref<Image> image;

	const uint8_t *r = p_buffer.ptr();
	int size = p_buffer.size();
	return basis_universal_unpacker_ptr(r, size);
}

void register_basis_universal_types() {
#ifdef TOOLS_ENABLED
	using namespace basisu;
	using namespace basist;
	basisu_encoder_init();
	Image::basis_universal_packer = basis_universal_packer;
#endif
	Image::basis_universal_unpacker = basis_universal_unpacker;
	Image::basis_universal_unpacker_ptr = basis_universal_unpacker_ptr;
}

void unregister_basis_universal_types() {
#ifdef TOOLS_ENABLED
	Image::basis_universal_packer = nullptr;
#endif
	Image::basis_universal_unpacker = nullptr;
	Image::basis_universal_unpacker_ptr = nullptr;
}

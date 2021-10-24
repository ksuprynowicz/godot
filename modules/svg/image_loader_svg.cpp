/*************************************************************************/
/*  image_loader_svg.cpp                                                 */
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

#include "image_loader_svg.h"

#include "core/error/error_macros.h"
#include "core/io/image_loader.h"
#include "core/templates/local_vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <memory>

#include "thirdparty/thorvg/inc/thorvg.h"

ImageLoaderSVG::ConvertTVGColorsFunc ImageLoaderSVG::convert_tvg_color_func = ImageLoaderSVG::_convert_tvg_paints;

void ImageLoaderSVG::set_convert_colors(Dictionary *p_replace_color) {
	if (p_replace_color) {
		Dictionary replace_color = *p_replace_color;
		for (int i = 0; i < replace_color.keys().size(); i++) {
			Variant o_c = replace_color.keys()[i];
			Variant n_c = replace_color[replace_color.keys()[i]];
			if (o_c.get_type() == Variant::COLOR && n_c.get_type() == Variant::COLOR) {
				Color old_color = o_c;
				Color new_color = n_c;
				replace_colors.old_colors.push_back(old_color.to_abgr32());
				replace_colors.new_colors.push_back(new_color.to_abgr32());
			}
		}
	} else {
		replace_colors.old_colors.clear();
		replace_colors.new_colors.clear();
	}
}

void ImageLoaderSVG::create_image_from_string(Ref<Image> p_image, String p_string, float p_scale, bool p_upsample, bool p_convert_color) {
	ERR_FAIL_COND(Math::is_zero_approx(p_scale));

	Vector<uint8_t> data = p_string.to_utf8_buffer();

	uint32_t bgColor = 0xffffffff;

	std::unique_ptr<tvg::Picture> picture = tvg::Picture::gen();

	float fw, fh;
	if (picture->load((const char *)data.ptr(), data.size()) != tvg::Result::Success) {
		return;
	}
	picture->viewbox(nullptr, nullptr, &fw, &fh);
	const float upscale = p_upsample ? 2.0 : 1.0;

	uint32_t width = MIN(fw * p_scale * upscale, 16 * 1024);
	uint32_t height = MIN(fh * p_scale * upscale, 16 * 1024);
	picture->size(width, height);

	std::unique_ptr<tvg::SwCanvas> sw_canvas = tvg::SwCanvas::gen();
	uint32_t *buffer = (uint32_t *)malloc(sizeof(uint32_t) * width * height);
	tvg::Result res = sw_canvas->target(buffer, width, width, height, tvg::SwCanvas::ARGB8888);
	if (res != tvg::Result::Success) {
		return;
	}

	if (bgColor != 0xffffffff) {
		uint8_t bgColorR = (uint8_t)((bgColor & 0xff0000) >> 16);
		uint8_t bgColorG = (uint8_t)((bgColor & 0x00ff00) >> 8);
		uint8_t bgColorB = (uint8_t)((bgColor & 0x0000ff));

		std::unique_ptr<tvg::Shape> shape = tvg::Shape::gen();
		shape->appendRect(0, 0, width, height, 0, 0); //x, y, w, h, rx, ry
		shape->fill(bgColorR, bgColorG, bgColorB, 255); //r, g, b, a

		if (sw_canvas->push(move(shape)) != tvg::Result::Success) {
			return;
		}
	}

	if (p_convert_color) {
		std::unique_ptr<tvg::Accessor>
				accessor = tvg::Accessor::gen();
		picture = accessor->access(move(picture), convert_tvg_color_func);
	}
	sw_canvas->push(move(picture));
	if (sw_canvas->draw() == tvg::Result::Success) {
		sw_canvas->sync();
	} else {
		return;
	}

	Vector<uint8_t> image;
	image.resize(width * height * sizeof(uint32_t));
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint32_t n = buffer[y * width + x];
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 0] = (n >> 16) & 0xff;
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 1] = (n >> 8) & 0xff;
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 2] = n & 0xff;
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 3] = (n >> 24) & 0xff;
		}
	}

	free(buffer);
	p_image->create(width, height, false, Image::FORMAT_RGBA8, image);
}

void ImageLoaderSVG::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("svg");
}

Error ImageLoaderSVG::load_image(Ref<Image> p_image, FileAccess *p_fileaccess, bool p_force_linear, float p_scale) {
	String svg = p_fileaccess->get_as_utf8_string();
	create_image_from_string(p_image, svg, p_scale, false, false);
	ERR_FAIL_COND_V(p_image->is_empty(), FAILED);
	if (p_force_linear) {
		p_image->srgb_to_linear();
	}
	return OK;
}

ImageLoaderSVG::ReplaceColors ImageLoaderSVG::replace_colors;
bool ImageLoaderSVG::_convert_tvg_paints(const tvg::Paint *p_paint) {
	union ByteColor {
		uint32_t i = 0;
		uint8_t b[sizeof(float)];
	};
	if (p_paint->identifier() == tvg::Shape::identifier()) {
		tvg::Shape *shape = (tvg::Shape *)p_paint;
		uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		uint8_t a = 0;
		shape->fillColor(&r, &g, &b, &a);
		for (int i = 0; i < replace_colors.old_colors.size(); i++) {
			ByteColor old_color = {};
			old_color.b[0] = r;
			old_color.b[1] = g;
			old_color.b[2] = b;
			old_color.b[3] = a;

			ByteColor new_color = {};
			new_color.i = replace_colors.new_colors[i];
			ByteColor replace_color = {};
			replace_color.i = replace_colors.new_colors[i];
			if (new_color.i == old_color.i) {
				shape->fill(replace_color.b[0], replace_color.b[1], replace_color.b[2],
						replace_color.b[3]);
				return true;
			}
		}
	} else if (p_paint->identifier() == tvg::LinearGradient::identifier() || p_paint->identifier() == tvg::RadialGradient::identifier()) {
		tvg::Shape *shape = (tvg::Shape *)p_paint;
		if (tvg::Fill *fill = shape->fill()) {
			tvg::Fill::ColorStop *colorStop;
			uint32_t count = fill->colorStops(&colorStop);
			for (int i = 0; i < count; ++i) {
				tvg::Fill::ColorStop &p = colorStop[i];
				for (int i = 0; i < replace_colors.old_colors.size(); i++) {
					ByteColor old_color = {};
					old_color.b[0] = p.r;
					old_color.b[1] = p.g;
					old_color.b[2] = p.b;
					old_color.b[3] = p.a;

					ByteColor new_color = {};
					new_color.i = replace_colors.new_colors[i];
					ByteColor replace_color = {};
					replace_color.i = replace_colors.new_colors[i];
					if (new_color.i == old_color.i) {
						p.r = replace_color.b[0];
						p.g = replace_color.b[1];
						p.b = replace_color.b[2];
						p.a = replace_color.b[3];
					}
				}
			}
		}
	}
	return true;
};
ImageLoaderSVG::ImageLoaderSVG() {
	tvg::CanvasEngine tvgEngine = tvg::CanvasEngine::Sw;
	uint32_t threads = std::thread::hardware_concurrency();
	if (threads > 0) {
		--threads;
	}
	if (tvg::Initializer::init(tvgEngine, threads) != tvg::Result::Success) {
		return;
	}
}

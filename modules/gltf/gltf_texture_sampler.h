/*************************************************************************/
/*  gltf_texture_sampler.h                                               */
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

#ifndef GLTF_TEXTURE_SAMPLER_H
#define GLTF_TEXTURE_SAMPLER_H

#include "core/io/resource.h"

class GLTFTextureSampler : public Resource {
	GDCLASS(GLTFTextureSampler, Resource);

public:
	enum {
		MAG_FILTER_NEAREST = 9728,
		MAG_FILTER_LINEAR = 9729
	};

	enum {
		MIN_FILTER_NEAREST = 9728,
		MIN_FILTER_LINEAR = 9729,
		MIN_FILTER_NEAREST_MIPMAP_NEAREST = 9984,
		MIN_FILTER_LINEAR_MIPMAP_NEAREST = 9985,
		MIN_FILTER_NEAREST_MIPMAP_LINEAR = 9986,
		MIN_FILTER_LINEAR_MIPMAP_LINEAR = 9987
	};

	enum {
		WRAP_MODE_REPEAT = 10497,
		WRAP_MODE_CLAMP_TO_EDGE = 33071,
		WRAP_MODE_MIRRORED_REPEAT = 33648,
	};

	int get_mag_filter() const {
		return mag_filter;
	};

	void set_mag_filter(const int filter_mode) {
		mag_filter = filter_mode;
	};

	int get_min_filter() const {
		return min_filter;
	};

	void set_min_filter(const int filter_mode) {
		min_filter = filter_mode;
	};

	int get_wrap_s() const {
		return wrap_s;
	};

	void set_wrap_s(const int wrap_mode) {
		wrap_s = wrap_mode;
	};

	int get_wrap_t() const {
		return wrap_t;
	};

	void set_wrap_t(const int wrap_mode) {
		wrap_s = wrap_mode;
	};

protected:
	static void _bind_methods();

private:
	int32_t mag_filter = MAG_FILTER_LINEAR;
	int32_t min_filter = MIN_FILTER_LINEAR_MIPMAP_LINEAR;
	int32_t wrap_s = WRAP_MODE_REPEAT;
	int32_t wrap_t = WRAP_MODE_REPEAT;
};

#endif
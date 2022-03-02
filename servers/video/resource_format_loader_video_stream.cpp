/*************************************************************************/
/*  resource_format_loader_video_stream.cpp                              */
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

#include "resource_format_loader_video_stream.h"

#include "core/object/class_db.h"
#include "servers/video_decoder_server.h"
#include "video_stream_extension.h"

RES ResourceFormatLoaderVideoStreamExtension::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	FileAccess *f = FileAccess::open(p_path, FileAccess::READ);
	if (!f) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return RES();
	}
	memdelete(f);

	Ref<VideoStreamExtension> stream = VideoDecoderServer::get_singleton()->get_extension_stream(p_path.get_extension());
	stream->set_file(p_path);
	if (r_error) {
		*r_error = OK;
	}
	return stream;
}

void ResourceFormatLoaderVideoStreamExtension::get_recognized_extensions(List<String> *p_extensions) const {
	auto recognized_extensions = VideoDecoderServer::get_singleton()->get_recognized_extensions();
	for (auto &ext : recognized_extensions) {
		p_extensions->push_back(ext);
	}
}

bool ResourceFormatLoaderVideoStreamExtension::handles_type(const String &p_type) const {
	return ClassDB::is_parent_class(p_type, "VideoStream");
}

String ResourceFormatLoaderVideoStreamExtension::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (VideoDecoderServer::get_singleton()->has_extension(el)) {
		return "VideoStreamExtension";
	}
	return "";
}

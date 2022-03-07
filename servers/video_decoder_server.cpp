/*************************************************************************/
/*  video_decoder_server.cpp                                             */
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

#include "video_decoder_server.h"
#include <servers/video/resource_format_loader_video_stream.h>

VideoDecoderServer *VideoDecoderServer::singleton = nullptr;
Ref<ResourceFormatLoader> VideoDecoderServer::resource_format_loader;

void VideoDecoderServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_recognized_extensions"), &VideoDecoderServer::get_recognized_extensions);
	ClassDB::bind_method(D_METHOD("add_interface", "extension_stream"), &VideoDecoderServer::add_interface);
	ClassDB::bind_method(D_METHOD("remove_interface", "extension_stream"), &VideoDecoderServer::remove_interface);
	ClassDB::bind_method(D_METHOD("get_extension_stream", "extension"), &VideoDecoderServer::get_extension_stream);
}

VideoDecoderServer *VideoDecoderServer::get_singleton() {
	return singleton;
}

bool VideoDecoderServer::has_extension(const String &ext) {
	return decoder_support.has(ext);
}

Vector<String> VideoDecoderServer::get_recognized_extensions() {
	return extensions;
}

void VideoDecoderServer::add_interface(const Ref<VideoStreamExtension> &extension_stream) {
	ERR_FAIL_COND(extension_stream.is_null());

	if (decoders.has(extension_stream)) {
		ERR_PRINT("Stream already Registered");
		return;
	}

	decoders.push_back(extension_stream);

	PackedStringArray supported_exts = extension_stream->get_supported_extensions();
	for (auto &new_ext : supported_exts) {
		if (!extensions.has(new_ext)) {
			extensions.push_back(new_ext);
		}
		decoder_support[new_ext].push_back(extension_stream);
	}

	reload_resource_loader();
}

void VideoDecoderServer::remove_interface(const Ref<VideoStreamExtension> &extension_stream) {
	int index = decoders.find(extension_stream);
	decoders.remove_at(index);

	PackedStringArray supported_ext = extension_stream->get_supported_extensions();
	for (auto &ext : supported_ext) {
		decoder_support[ext].erase(extension_stream);
		if (decoder_support[ext].is_empty()) {
			extensions.erase(ext);
			decoder_support.erase(ext);
		}
	}

	reload_resource_loader();
}

Ref<VideoStreamExtension> VideoDecoderServer::get_extension_stream(const String &extension) {
	if (extensions.size() == 0 || !extensions.has(extension)) {
		return nullptr;
	}
	return decoder_support[extension][0];
}

VideoDecoderServer::VideoDecoderServer() {
	singleton = this;
	resource_format_loader.reference_ptr(memnew(ResourceFormatLoaderVideoStreamExtension));
	ResourceLoader::add_resource_format_loader(resource_format_loader, true);
}

VideoDecoderServer::~VideoDecoderServer() {
	singleton = nullptr;
	ResourceLoader::remove_resource_format_loader(resource_format_loader);
	resource_format_loader.unref();
}

void VideoDecoderServer::register_resource_loader() {
	ResourceLoader::add_resource_format_loader(resource_format_loader, true);
}

void VideoDecoderServer::unregister_resource_loader() {
	ResourceLoader::remove_resource_format_loader(resource_format_loader);
}

void VideoDecoderServer::reload_resource_loader() {
	unregister_resource_loader();
	register_resource_loader();
}

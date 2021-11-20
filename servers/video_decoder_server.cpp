
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

	int decoder_index = decoders.size();
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
	decoders.remove(index);

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

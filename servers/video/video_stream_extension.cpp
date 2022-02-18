/*************************************************************************/
/*  video_stream_extension.cpp                                           */
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

#include "video_stream_extension.h"

#include "core/config/project_settings.h"
#include "servers/audio_server.h"

const int AUX_BUFFER_SIZE = 1024; // Buffer 1024 samples.

// VideoStreamPlaybackExtension starts here.

void VideoStreamPlaybackExtension::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("open_file"), &VideoStreamPlaybackExtension::open_file);
	ClassDB::bind_method(D_METHOD("file_read", "data"), &VideoStreamPlaybackExtension::file_read);
	ClassDB::bind_method(D_METHOD("file_seek", "pos"), &VideoStreamPlaybackExtension::file_seek);
	ClassDB::bind_method(D_METHOD("file_pos"), &VideoStreamPlaybackExtension::file_pos);
	ClassDB::bind_method(D_METHOD("file_len"), &VideoStreamPlaybackExtension::file_len);
	GDVIRTUAL_BIND(_stop);
	GDVIRTUAL_BIND(_play);
	GDVIRTUAL_BIND(_is_playing);
	GDVIRTUAL_BIND(_set_paused, "paused");
	GDVIRTUAL_BIND(_is_paused);
	GDVIRTUAL_BIND(_set_loop, "enable");
	GDVIRTUAL_BIND(_has_loop);
	GDVIRTUAL_BIND(_get_length);
	GDVIRTUAL_BIND(_get_playback_position);
	GDVIRTUAL_BIND(_seek, "time");
	GDVIRTUAL_BIND(_set_audio_track, "idx");
	GDVIRTUAL_BIND(_get_texture);
	GDVIRTUAL_BIND(_update, "delta");
	GDVIRTUAL_BIND(_get_channels);
	GDVIRTUAL_BIND(_get_mix_rate);
	GDVIRTUAL_BIND(_initialize);
	GDVIRTUAL_BIND(_cleanup);
}

VideoStreamPlaybackExtension::VideoStreamPlaybackExtension() {
	GDVIRTUAL_CALL(_initialize);
}

VideoStreamPlaybackExtension::~VideoStreamPlaybackExtension() {
	GDVIRTUAL_CALL(_cleanup);
}

void VideoStreamPlaybackExtension::stop() {
	GDVIRTUAL_CALL(_stop);
}

void VideoStreamPlaybackExtension::play() {
	GDVIRTUAL_CALL(_play);
}

bool VideoStreamPlaybackExtension::is_playing() const {
	bool ret;
	if (GDVIRTUAL_CALL(_is_playing, ret)) {
		return ret;
	}
	return false;
}

void VideoStreamPlaybackExtension::set_paused(bool p_paused) {
	GDVIRTUAL_CALL(_is_playing, p_paused);
}

bool VideoStreamPlaybackExtension::is_paused() const {
	bool ret;
	if (GDVIRTUAL_CALL(_is_paused, ret)) {
		return ret;
	}
	return false;
}

void VideoStreamPlaybackExtension::set_loop(bool p_enable) {
	GDVIRTUAL_CALL(_set_loop, p_enable);
}

bool VideoStreamPlaybackExtension::has_loop() const {
	bool ret;
	if (GDVIRTUAL_CALL(_has_loop, ret)) {
		return ret;
	}
	return false;
}

float VideoStreamPlaybackExtension::get_length() const {
	float ret;
	if (GDVIRTUAL_CALL(_get_length, ret)) {
		return ret;
	}
	return 0;
}

float VideoStreamPlaybackExtension::get_playback_position() const {
	float ret;
	if (GDVIRTUAL_CALL(_get_playback_position, ret)) {
		return ret;
	}
	return 0;
}

void VideoStreamPlaybackExtension::seek(float p_time) {
	GDVIRTUAL_CALL(_seek, p_time);
}

void VideoStreamPlaybackExtension::set_audio_track(int p_idx) {
	GDVIRTUAL_CALL(_set_audio_track, p_idx);
}

Ref<Texture2D> VideoStreamPlaybackExtension::get_texture() const {
	Ref<Texture2D> ret;
	if (GDVIRTUAL_CALL(_get_texture, ret)) {
		return ret;
	}
	return nullptr;
}

void VideoStreamPlaybackExtension::update(float p_delta) {
	GDVIRTUAL_CALL(_update, p_delta);
}

void VideoStreamPlaybackExtension::set_mix_callback(AudioMixCallback p_callback, void *p_userdata) {
	WARN_DEPRECATED;
}

int VideoStreamPlaybackExtension::get_channels() const {
	int ret;
	if (GDVIRTUAL_CALL(_get_channels, ret)) {
		return ret;
	}
	return 0;
}

int VideoStreamPlaybackExtension::get_mix_rate() const {
	int ret;
	if (GDVIRTUAL_CALL(_get_mix_rate, ret)) {
		return ret;
	}
	return 0;
}

bool VideoStreamPlaybackExtension::open_file(const String &p_file) {
	file = FileAccess::open(p_file, FileAccess::READ);

	return file != nullptr;
}

uint64_t VideoStreamPlaybackExtension::file_read(PackedByteArray data) {
	return file->get_buffer(data.ptrw(), data.size());
}

void VideoStreamPlaybackExtension::file_seek(int pos) {
	file->seek(pos);
}

uint64_t VideoStreamPlaybackExtension::file_pos() {
	return file->get_position();
}

uint64_t VideoStreamPlaybackExtension::file_len() {
	return file->get_length();
}

/* --- NOTE VideoStreamExtension starts here. ----- */

Ref<VideoStreamPlayback> VideoStreamExtension::instance_playback() {
	Ref<VideoStreamPlaybackExtension> ref;
	if (GDVIRTUAL_CALL(_instance_playback, ref)) {
		ERR_FAIL_COND_V_MSG(ref.is_null(), nullptr, "Plugin returned null playback");
		const bool file_opened = ref->open_file(file);
		ref->set_audio_track(audio_track);
		return ref;
	}
	return nullptr;
}

PackedStringArray VideoStreamExtension::get_supported_extensions() const {
	PackedStringArray pbr;
	if (GDVIRTUAL_CALL(_get_supported_extensions, pbr)) {
		return pbr;
	}
	return {};
}

VideoStreamExtension::VideoStreamExtension() {
	GDVIRTUAL_CALL(_initialize);
}

VideoStreamExtension::~VideoStreamExtension() {
	GDVIRTUAL_CALL(_cleanup);
}

void VideoStreamExtension::set_file(const String &p_file) {
	file = p_file;
}

String VideoStreamExtension::get_file() {
	return file;
}

void VideoStreamExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_file", "file"), &VideoStreamExtension::set_file);
	ClassDB::bind_method(D_METHOD("get_file"), &VideoStreamExtension::get_file);

	ClassDB::bind_method(D_METHOD("set_audio_track", "track"), &VideoStreamExtension::set_audio_track);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "file", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "set_file", "get_file");

	GDVIRTUAL_BIND(_get_supported_extensions);
	GDVIRTUAL_BIND(_instance_playback);
	GDVIRTUAL_BIND(_initialize);
	GDVIRTUAL_BIND(_cleanup);
}

void VideoStreamExtension::set_audio_track(int p_track) {
	audio_track = p_track;
}

/* --- NOTE ResourceFormatLoaderVideoStreamExtension starts here. ----- */

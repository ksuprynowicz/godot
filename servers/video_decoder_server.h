/*************************************************************************/
/*  video_decoder_server.h                                               */
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

#ifndef VIDEO_DECODER_SERVER_H
#define VIDEO_DECODER_SERVER_H

#include "servers/video/video_stream_extension.h"

class VideoDecoderServer : public Object {
	GDCLASS(VideoDecoderServer, Object);
	_THREAD_SAFE_CLASS_
	Vector<Ref<VideoStreamExtension>> decoders;
	Vector<String> extensions;
	Map<String, Vector<Ref<VideoStreamExtension>>> decoder_support;

protected:
	static VideoDecoderServer *singleton;
	static Ref<ResourceFormatLoader> resource_format_loader;

	static void _bind_methods();
	void register_resource_loader();
	void unregister_resource_loader();
	void reload_resource_loader();

public:
	static VideoDecoderServer *get_singleton();

	bool has_extension(const String &ext);
	Vector<String> get_recognized_extensions();

	void add_interface(const Ref<VideoStreamExtension> &extension_stream);
	void remove_interface(const Ref<VideoStreamExtension> &extension_stream);

	Ref<VideoStreamExtension> get_extension_stream(const String &extension);

	VideoDecoderServer();

	virtual ~VideoDecoderServer();
};

#endif // VIDEO_DECODER_SERVER_H

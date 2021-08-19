/*************************************************************************/
/*  resource_importer_bmont.cpp                                          */
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

#include "resource_importer_bmfont.h"

#include "core/io/resource_saver.h"

String ResourceImporterBMFont::get_importer_name() const {
	return "font_data_bmfont";
}

String ResourceImporterBMFont::get_visible_name() const {
	return "Font Data (BMFont)";
}

void ResourceImporterBMFont::get_recognized_extensions(List<String> *p_extensions) const {
	if (p_extensions) {
		p_extensions->push_back("font");
		p_extensions->push_back("fnt");
	}
}

String ResourceImporterBMFont::get_save_extension() const {
	return "fontdata";
}

String ResourceImporterBMFont::get_resource_type() const {
	return "FontData";
}

bool ResourceImporterBMFont::get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const {
	return true;
}

void ResourceImporterBMFont::get_import_options(List<ImportOption> *r_options, int p_preset) const {
	// No options.
}

Error ResourceImporterBMFont::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	print_verbose("Importing raw image font from: " + p_source_file);

	Ref<FontData> font;
	font.instantiate();
	font->set_antialiased(false);
	font->set_multichannel_signed_distance_field(false);
	font->set_force_autohinter(false);
	font->set_hinting(TextServer::HINTING_NONE);
	font->set_oversampling(1.0f);

	FileAccessRef f = FileAccess::open(p_source_file, FileAccess::READ);
	if (f == nullptr) {
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, TTR("Cannot open font from file ") + "\"" + p_source_file + "\".");
	}

	int base_size = 16;
	int height = 0;
	int ascent = 0;
	int page = 0;

	while (true) {
		String line = f->get_line();

		int delimiter = line.find(" ");
		String type = line.substr(0, delimiter);
		int pos = delimiter + 1;
		Map<String, String> keys;

		while (pos < line.size() && line[pos] == ' ') {
			pos++;
		}

		while (pos < line.size()) {
			int eq = line.find("=", pos);
			if (eq == -1) {
				break;
			}
			String key = line.substr(pos, eq - pos);
			int end = -1;
			String value;
			if (line[eq + 1] == '"') {
				end = line.find("\"", eq + 2);
				if (end == -1) {
					break;
				}
				value = line.substr(eq + 2, end - 1 - eq - 1);
				pos = end + 1;
			} else {
				end = line.find(" ", eq + 1);
				if (end == -1) {
					end = line.size();
				}
				value = line.substr(eq + 1, end - eq);
				pos = end;
			}

			while (pos < line.size() && line[pos] == ' ') {
				pos++;
			}

			keys[key] = value;
		}

		if (type == "info") {
			if (keys.has("size")) {
				base_size = keys["size"].to_int();
				font->set_fixed_size(base_size);
			}
		} else if (type == "common") {
			if (keys.has("lineHeight")) {
				height = keys["lineHeight"].to_int();
			}
			if (keys.has("base")) {
				ascent = keys["base"].to_int();
			}
		} else if (type == "page") {
			if (keys.has("file")) {
				String base_dir = p_source_file.get_base_dir();
				String file = base_dir.plus_file(keys["file"]);
				if (RenderingServer::get_singleton() != nullptr) {
					Ref<Image> img;
					img.instantiate();
					Error err = img->load(file);
					ERR_FAIL_COND_V_MSG(err != OK, ERR_FILE_CANT_READ, TTR("Can't load font texture: ") + "\"" + file + "\".");
					font->set_texture_image_format(0, Vector2i(base_size, 0), page, img->get_format());
					font->set_texture_image_size(0, Vector2i(base_size, 0), page, img->get_size());
					font->set_texture_image_data(0, Vector2i(base_size, 0), page, img->get_data());
				}
				page++;
			}
		} else if (type == "char") {
			char32_t idx = 0;
			Vector2 advance;
			Vector2 size;
			Vector2 offset;
			Rect2 uv_rect;
			int texture_idx = -1;

			if (keys.has("id")) {
				idx = keys["id"].to_int();
			}
			if (keys.has("x")) {
				uv_rect.position.x = keys["x"].to_int();
			}
			if (keys.has("y")) {
				uv_rect.position.y = keys["y"].to_int();
			}
			if (keys.has("width")) {
				uv_rect.size.width = keys["width"].to_int();
				size.width = keys["width"].to_int();
			}
			if (keys.has("height")) {
				uv_rect.size.height = keys["height"].to_int();
				size.height = keys["height"].to_int();
			}
			if (keys.has("xoffset")) {
				offset.x = keys["xoffset"].to_int();
			}
			if (keys.has("yoffset")) {
				offset.y = keys["yoffset"].to_int() - ascent;
			}
			if (keys.has("page")) {
				texture_idx = keys["page"].to_int();
			}
			if (keys.has("xadvance")) {
				advance.x = keys["xadvance"].to_int();
			}
			if (keys.has("yadvance")) {
				advance.y = keys["yadvance"].to_int();
			}
			if (advance.x < 0) {
				advance.x = size.width + 1;
			}
			if (advance.y < 0) {
				advance.y = size.height + 1;
			}
			font->set_glyph_advance(0, base_size, idx, advance);
			font->set_glyph_offset(0, Vector2i(base_size, 0), idx, offset);
			font->set_glyph_size(0, Vector2i(base_size, 0), idx, size);
			font->set_glyph_uv_rect(0, Vector2i(base_size, 0), idx, uv_rect);
			font->set_glyph_texture_idx(0, Vector2i(base_size, 0), idx, texture_idx);
		} else if (type == "kerning") {
			Vector2i kpk;
			if (keys.has("first")) {
				kpk.x = keys["first"].to_int();
			}
			if (keys.has("second")) {
				kpk.y = keys["second"].to_int();
			}
			if (keys.has("amount")) {
				font->set_kerning(0, base_size, kpk, Vector2(keys["amount"].to_int(), 0));
			}
		}

		if (f->eof_reached()) {
			break;
		}
	}
	font->set_ascent(0, base_size, ascent);
	font->set_descent(0, base_size, height - ascent);

	print_verbose("Saving to: " + p_save_path + ".fontdata");
	Error err = ResourceSaver::save(p_save_path + ".fontdata", font);
	ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot save font to file \"" + p_save_path + ".res\".");

	return OK;
}

ResourceImporterBMFont::ResourceImporterBMFont() {
}

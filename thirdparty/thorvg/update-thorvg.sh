rm -rf AUTHORS  inc  LICENSE  src *.zip
curl -L -O https://github.com/Samsung/thorvg/archive/refs/tags/v0.7.0.zip
bsdtar --strip-components=1 -xvf  *.zip
rm *.zip
rm -rf .github docs pc res test tools .git* *.md *.txt wasm_build.sh
find . -type f -name 'meson.build' -delete
rm -rf src/bin src/bindings src/examples src/wasm
rm -rf src/lib/gl_engine tvgcompat
mkdir -p inc
cat << 'EOF' > inc/config.h
/*************************************************************************/
/*  config.h                                                             */
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

#ifndef SVG_CONFIG_H
#define SVG_CONFIG_H

#define THORVG_SW_RASTER_SUPPORT 1

#define THORVG_SVG_LOADER_SUPPORT 1

#define THORVG_PNG_LOADER_SUPPORT 1

#define THORVG_TVG_LOADER_SUPPORT 1

#define THORVG_TVG_SAVER_SUPPORT 1

#define THORVG_JPG_LOADER_SUPPORT 1

#define THORVG_VERSION_STRING "0.7.0"
#endif
EOF

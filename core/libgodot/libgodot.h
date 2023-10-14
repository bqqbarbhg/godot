/**************************************************************************/
/*  libgodot.h                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef LIBGODOT_H
#define LIBGODOT_H

#if defined(WINDOWS_ENABLED) | defined(UWP_ENABLED)
#define LIBGODOT_API __declspec(dllexport)
#elif defined(ANDROID_ENABLED)
#include <jni.h>
#define LIBGODOT_API JNIEXPORT
#elif defined(X11_ENABLED) | defined(LINUXBSD_ENABLED) | defined(MACOS_ENABLED) | defined(IOS_ENABLED)
#define LIBGODOT_API __attribute__((visibility("default")))
#elif defined(WEB_ENABLED)
#include <emscripten/emscripten.h>
#define LIBGODOT_API extern EMSCRIPTEN_KEEPALIVE
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LIBGODOT_API) 

// True means it failed and should fallback to os
LIBGODOT_API void libgodot_bind_open_dynamic_library(bool (*open_dynamic_library_bind)(const char *, void **, bool, const char **));
LIBGODOT_API void libgodot_bind_close_dynamic_library(bool (*close_dynamic_library_bind)(void *));
LIBGODOT_API void libgodot_bind_get_dynamic_library_symbol_handle(bool (*get_dynamic_library_symbol_handle_bind)(void *, const char *, void **, bool));

#if !(defined(WEB_ENABLED) | defined(ANDROID_ENABLED))
LIBGODOT_API int godot_main(int argc, char *argv[]);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif // LIBGODOT_H

/**************************************************************************/
/*  libgodot.cpp                                                          */
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

#include "libgodot.h"
#include "libgodot_internal.h"

bool (*open_dynamic_library_bound)(const char *, void **, bool, const char **);
bool (*close_dynamic_library_bound)(void *);
bool (*get_dynamic_library_symbol_handle_bound)(void *, const char *, void **, bool);

bool libgodot_open_dynamic_library(const String p_path, void *&p_library_handle, bool p_also_set_library_path, String *r_resolved_path) {
	if (open_dynamic_library_bound == nullptr) {
		return false;
	}

	void *handle = nullptr;
	if (r_resolved_path != nullptr) {
		const char *r_path = nullptr;
		if (open_dynamic_library_bound(p_path.ascii().get_data(), &handle, p_also_set_library_path, &r_path)) {
			if (handle == nullptr) {
				return false;
			}
			if (r_path == nullptr) {
				return false;
			}
			r_resolved_path[0] = String(r_path);
			p_library_handle = handle;
			return true;
		}
	} else {
		if (open_dynamic_library_bound(p_path.ascii().get_data(), &handle, p_also_set_library_path, nullptr)) {
			if (handle == nullptr) {
				return false;
			}
			p_library_handle = handle;
			return true;
		}
	}

	return false;
}

bool libgodot_close_dynamic_library(void *p_library_handle) {
	if (close_dynamic_library_bound == nullptr) {
		return false;
	}

	if (close_dynamic_library_bound(p_library_handle)) {
		return true;
	}

	return false;
}

bool libgodot_get_dynamic_library_symbol_handle(void *p_library_handle, const String p_name, void *&p_symbol_handle, bool p_optional) {
	if (get_dynamic_library_symbol_handle_bound == nullptr) {
		return false;
	}

	void *symbol_handle = nullptr;
	if (get_dynamic_library_symbol_handle_bound(p_library_handle, p_name.ascii().get_data(), &symbol_handle, p_optional)) {
		if (symbol_handle == nullptr) {
			return false;
		}
		p_symbol_handle = symbol_handle;
		return true;
	}

	return false;
}

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LIBGODOT_API)

LIBGODOT_API void libgodot_bind_open_dynamic_library(bool (*open_dynamic_library_bind)(const char *, void **, bool, const char **)) {
	open_dynamic_library_bound = open_dynamic_library_bind;
}

LIBGODOT_API void libgodot_bind_close_dynamic_library(bool (*close_dynamic_library_bind)(void *)) {
	close_dynamic_library_bound = close_dynamic_library_bind;
}

LIBGODOT_API void libgodot_bind_get_dynamic_library_symbol_handle(bool (*get_dynamic_library_symbol_handle_bind)(void *, const char *, void **, bool)) {
	get_dynamic_library_symbol_handle_bound = get_dynamic_library_symbol_handle_bind;
}

#endif

#ifdef __cplusplus
}

#endif

/*************************************************************************/
/*  gltf_document_extension.cpp                                          */
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

#include "gltf_document_extension.h"

#include "gltf_document.h"

void GLTFDocumentExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_import_settings", "settings"),
			&GLTFDocumentExtension::set_import_settings);
	ClassDB::bind_method(D_METHOD("set_export_settings", "settings"),
			&GLTFDocumentExtension::set_export_settings);
	ClassDB::bind_method(D_METHOD("get_import_settings"),
			&GLTFDocumentExtension::get_import_settings);
	ClassDB::bind_method(D_METHOD("get_export_settings"),
			&GLTFDocumentExtension::get_export_settings);
	ClassDB::bind_method(D_METHOD("import_preflight", "document", "settings"),
			&GLTFDocumentExtension::import_preflight);
	ClassDB::bind_method(D_METHOD("import_post", "document", "node", "settings"),
			&GLTFDocumentExtension::import_post);
	ClassDB::bind_method(D_METHOD("export_preflight", "document", "node", "settings"),
			&GLTFDocumentExtension::export_preflight);
	ClassDB::bind_method(D_METHOD("export_post", "document", "settings"),
			&GLTFDocumentExtension::export_post);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "import_settings"), "set_import_settings", "get_import_settings");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "export_settings"), "set_export_settings", "get_export_settings");
}

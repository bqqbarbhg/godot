/**************************************************************************/
/*  kdtree_wrapper.h                                                      */
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

#ifndef KDTREE_WRAPPER_H
#define KDTREE_WRAPPER_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/string/node_path.h"
#include "core/string/print_string.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"

#include <chrono>

#include "../thirdparty/kdtree.hpp"

struct KDTree : public Resource {
	GDCLASS(KDTree, Resource)

	Kdtree::KdTree *kd = nullptr;
	KDTree() :
			kd{ nullptr } {
	}
	~KDTree() {
	}
	void set_dimension(int dim);
	void bake_nodes(const PackedFloat32Array &points, int64_t dimensions);
	void set_weight(int difference_type, PackedFloat32Array weight);
	PackedInt32Array k_nearest_neighbors(PackedFloat32Array point, int64_t k);
	PackedInt32Array range_nearest_neighbors(PackedFloat32Array point, int64_t k);

protected:
	static void _bind_methods();
};

#endif // KDTREE_WRAPPER_H
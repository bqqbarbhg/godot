/**************************************************************************/
/*  fbx_defines.h                                                         */
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

#ifndef FBX_DEFINES_H
#define FBX_DEFINES_H

// This file should only be included by other headers.

// Godot classes used by GLTF headers.
class BoneAttachment3D;
class CSGShape3D;
class GridMap;
class ImporterMeshInstance3D;
class Light3D;
class MeshInstance3D;
class MultiMeshInstance3D;
class Skeleton3D;
class Skin;

// GLTF classes.
class FBXAnimation;
class FBXDocument;
class FBXDocumentExtension;
class FBXMesh;
class FBXNode;
class FBXSkeleton;
class FBXSkin;
class FBXState;
class FBXTexture;

// GLTF index aliases.
using FBXAnimationIndex = int;
using FBXBufferIndex = int;
using FBXImageIndex = int;
using FBXMaterialIndex = int;
using FBXMeshIndex = int;
using FBXNodeIndex = int;
using FBXSkeletonIndex = int;
using FBXSkinIndex = int;
using FBXTextureIndex = int;

enum FBXType {
	TYPE_SCALAR,
	TYPE_VEC2,
	TYPE_VEC3,
	TYPE_VEC4,
	TYPE_MAT2,
	TYPE_MAT3,
	TYPE_MAT4,
};

#endif // FBX_DEFINES_H

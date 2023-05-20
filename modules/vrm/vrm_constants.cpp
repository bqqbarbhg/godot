/**************************************************************************/
/*  vrm_constants.cpp                                                     */
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

#include "vrm_constants.h"

void VRMConstants::_bind_methods() {
}

VRMConstants::VRMConstants(bool is_vrm_0) {
	vrm_to_human_bone.insert("hips", "Hips");
	vrm_to_human_bone.insert("spine", "Spine");
	vrm_to_human_bone.insert("chest", "Chest");
	vrm_to_human_bone.insert("upperChest", "UpperChest");
	vrm_to_human_bone.insert("neck", "Neck");
	vrm_to_human_bone.insert("head", "Head");
	vrm_to_human_bone.insert("leftEye", "LeftEye");
	vrm_to_human_bone.insert("rightEye", "RightEye");
	vrm_to_human_bone.insert("jaw", "Jaw");
	vrm_to_human_bone.insert("leftShoulder", "LeftShoulder");
	vrm_to_human_bone.insert("leftUpperArm", "LeftUpperArm");
	vrm_to_human_bone.insert("leftLowerArm", "LeftLowerArm");
	vrm_to_human_bone.insert("leftHand", "LeftHand");
	vrm_to_human_bone.insert("leftThumbMetacarpal", "LeftThumbMetacarpal");
	vrm_to_human_bone.insert("leftThumbProximal", "LeftThumbProximal");
	vrm_to_human_bone.insert("leftThumbDistal", "LeftThumbDistal");
	vrm_to_human_bone.insert("leftIndexProximal", "LeftIndexProximal");
	vrm_to_human_bone.insert("leftIndexIntermediate", "LeftIndexIntermediate");
	vrm_to_human_bone.insert("leftIndexDistal", "LeftIndexDistal");
	vrm_to_human_bone.insert("leftMiddleProximal", "LeftMiddleProximal");
	vrm_to_human_bone.insert("leftMiddleIntermediate", "LeftMiddleIntermediate");
	vrm_to_human_bone.insert("leftMiddleDistal", "LeftMiddleDistal");
	vrm_to_human_bone.insert("leftRingProximal", "LeftRingProximal");
	vrm_to_human_bone.insert("leftRingIntermediate", "LeftRingIntermediate");
	vrm_to_human_bone.insert("leftRingDistal", "LeftRingDistal");
	vrm_to_human_bone.insert("leftLittleProximal", "LeftLittleProximal");
	vrm_to_human_bone.insert("leftLittleIntermediate", "LeftLittleIntermediate");
	vrm_to_human_bone.insert("leftLittleDistal", "LeftLittleDistal");
	vrm_to_human_bone.insert("rightShoulder", "RightShoulder");
	vrm_to_human_bone.insert("rightUpperArm", "RightUpperArm");
	vrm_to_human_bone.insert("rightLowerArm", "RightLowerArm");
	vrm_to_human_bone.insert("rightHand", "RightHand");
	vrm_to_human_bone.insert("rightThumbMetacarpal", "RightThumbMetacarpal");
	vrm_to_human_bone.insert("rightThumbProximal", "RightThumbProximal");
	vrm_to_human_bone.insert("rightThumbDistal", "RightThumbDistal");
	vrm_to_human_bone.insert("rightIndexProximal", "RightIndexProximal");
	vrm_to_human_bone.insert("rightIndexIntermediate", "RightIndexIntermediate");
	vrm_to_human_bone.insert("rightIndexDistal", "RightIndexDistal");
	vrm_to_human_bone.insert("rightMiddleProximal", "RightMiddleProximal");
	vrm_to_human_bone.insert("rightMiddleIntermediate", "RightMiddleIntermediate");
	vrm_to_human_bone.insert("rightMiddleDistal", "RightMiddleDistal");
	vrm_to_human_bone.insert("rightRingProximal", "RightRingProximal");
	vrm_to_human_bone.insert("rightRingIntermediate", "RightRingIntermediate");
	vrm_to_human_bone.insert("rightRingDistal", "RightRingDistal");
	vrm_to_human_bone.insert("rightLittleProximal", "RightLittleProximal");
	vrm_to_human_bone.insert("rightLittleIntermediate", "RightLittleIntermediate");
	vrm_to_human_bone.insert("rightLittleDistal", "RightLittleDistal");
	vrm_to_human_bone.insert("leftUpperLeg", "LeftUpperLeg");
	vrm_to_human_bone.insert("leftLowerLeg", "LeftLowerLeg");
	vrm_to_human_bone.insert("leftFoot", "LeftFoot");
	vrm_to_human_bone.insert("leftToes", "LeftToes");
	vrm_to_human_bone.insert("rightUpperLeg", "RightUpperLeg");
	vrm_to_human_bone.insert("rightLowerLeg", "RightLowerLeg");
	vrm_to_human_bone.insert("rightFoot", "RightFoot");
	vrm_to_human_bone.insert("rightToes", "RightToes");
	if (is_vrm_0) {
		vrm_to_human_bone["leftThumbIntermediate"] = "LeftThumbProximal";
		vrm_to_human_bone["leftThumbProximal"] = "LeftThumbMetacarpal";
		vrm_to_human_bone["rightThumbIntermediate"] = "RightThumbProximal";
		vrm_to_human_bone["rightThumbProximal"] = "RightThumbMetacarpal";
	}
	for (const KeyValue<String, String> &E : vrm_to_human_bone) {
		vrm_to_human_bone_dict[E.key] = E.value;
	}
}


#ifndef VRM_VRMSPRINGBONE_H
#define VRM_VRMSPRINGBONE_H

#include "core/io/resource.h"
#include "scene/3d/skeleton_3d.h"
#include "vrm_spring_bone_logic.h"

class VRMSpringBone : public Resource {
	GDCLASS(VRMSpringBone, Resource);

public:
	// Annotation comment
	String comment;

	// The resilience of the swaying object (the power of returning to the initial pose).
	float stiffness_force = 1.0f;
	// The strength of gravity.
	float gravity_power = 0.0f;

	// The direction of gravity.
	Vector3 gravity_dir = Vector3(0.0, -1.0, 0.0);

	// The resistance (deceleration) of automatic animation.
	float drag_force = 0.4f;

	// Bone name references are only valid within a given Skeleton.
	NodePath skeleton;

	// The reference point of a swaying object can be set at any location except the origin.
	String center_bone = "";
	NodePath center_node;

	// The radius of the sphere used for the collision detection with colliders.
	float hit_radius = 0.02f;

	// bone name of the root bone of the swaying object, within skeleton.
	Array root_bones;

	// Reference to the vrm_collidergroup for collisions with swaying objects.
	Array collider_groups;

	Array verlets;
	Array colliders;
	Vector3 center;
	Skeleton3D *skel = nullptr;

public:
	String get_comment() const {
		return comment;
	}

	void set_comment(const String &p_comment) {
		comment = p_comment;
	}

	float get_stiffness_force() const {
		return stiffness_force;
	}

	void set_stiffness_force(float p_stiffness_force) {
		stiffness_force = p_stiffness_force;
	}

	float get_gravity_power() const {
		return gravity_power;
	}

	void set_gravity_power(float p_gravity_power) {
		gravity_power = p_gravity_power;
	}

	Vector3 get_gravity_dir() const {
		return gravity_dir;
	}

	void set_gravity_dir(const Vector3 &p_gravity_dir) {
		gravity_dir = p_gravity_dir;
	}

	float get_drag_force() const {
		return drag_force;
	}

	void set_drag_force(float p_drag_force) {
		drag_force = p_drag_force;
	}

	NodePath get_skeleton() const {
		return skeleton;
	}

	void set_skeleton(const NodePath &p_skeleton) {
		skeleton = p_skeleton;
	}

	String get_center_bone() const {
		return center_bone;
	}

	void set_center_bone(const String &p_center_bone) {
		center_bone = p_center_bone;
	}

	NodePath get_center_node() const {
		return center_node;
	}

	void set_center_node(const NodePath &p_center_node) {
		center_node = p_center_node;
	}

	float get_hit_radius() const {
		return hit_radius;
	}

	void set_hit_radius(float p_hit_radius) {
		hit_radius = p_hit_radius;
	}

	Array get_root_bones() const {
		return root_bones;
	}

	void set_root_bones(const Array &p_root_bones) {
		root_bones = p_root_bones;
	}

	Array get_collider_groups() const {
		return collider_groups;
	}

	void set_collider_groups(const Array &p_collider_groups) {
		collider_groups = p_collider_groups;
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_comment"), &VRMSpringBone::get_comment);
		ClassDB::bind_method(D_METHOD("set_comment", "value"), &VRMSpringBone::set_comment);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "comment", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_comment", "get_comment");

		ClassDB::bind_method(D_METHOD("get_stiffness_force"), &VRMSpringBone::get_stiffness_force);
		ClassDB::bind_method(D_METHOD("set_stiffness_force", "value"), &VRMSpringBone::set_stiffness_force);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stiffness_force", PROPERTY_HINT_RANGE, "0,4,0.01", PROPERTY_USAGE_DEFAULT), "set_stiffness_force", "get_stiffness_force");

		// Add other exported variables here using the same pattern
		ClassDB::bind_method(D_METHOD("get_gravity_power"), &VRMSpringBone::get_gravity_power);
		ClassDB::bind_method(D_METHOD("set_gravity_power", "value"), &VRMSpringBone::set_gravity_power);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_power", PROPERTY_HINT_RANGE, "0,10,0.1", PROPERTY_USAGE_DEFAULT), "set_gravity_power", "get_gravity_power");

		ClassDB::bind_method(D_METHOD("get_gravity_dir"), &VRMSpringBone::get_gravity_dir);
		ClassDB::bind_method(D_METHOD("set_gravity_dir", "value"), &VRMSpringBone::set_gravity_dir);
		ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gravity_dir", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_gravity_dir", "get_gravity_dir");

		ClassDB::bind_method(D_METHOD("get_drag_force"), &VRMSpringBone::get_drag_force);
		ClassDB::bind_method(D_METHOD("set_drag_force", "value"), &VRMSpringBone::set_drag_force);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drag_force", PROPERTY_HINT_RANGE, "0,4,0.01", PROPERTY_USAGE_DEFAULT), "set_drag_force", "get_drag_force");

		ClassDB::bind_method(D_METHOD("get_skeleton"), &VRMSpringBone::get_skeleton);
		ClassDB::bind_method(D_METHOD("set_skeleton", "value"), &VRMSpringBone::set_skeleton);
		ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_skeleton", "get_skeleton");

		ClassDB::bind_method(D_METHOD("get_center_bone"), &VRMSpringBone::get_center_bone);
		ClassDB::bind_method(D_METHOD("set_center_bone", "value"), &VRMSpringBone::set_center_bone);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "center_bone", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_center_bone", "get_center_bone");

		ClassDB::bind_method(D_METHOD("get_center_node"), &VRMSpringBone::get_center_node);
		ClassDB::bind_method(D_METHOD("set_center_node", "value"), &VRMSpringBone::set_center_node);
		ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "center_node", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_center_node", "get_center_node");

		ClassDB::bind_method(D_METHOD("get_hit_radius"), &VRMSpringBone::get_hit_radius);
		ClassDB::bind_method(D_METHOD("set_hit_radius", "value"), &VRMSpringBone::set_hit_radius);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hit_radius", PROPERTY_HINT_RANGE, "0,10,0.1", PROPERTY_USAGE_DEFAULT), "set_hit_radius", "get_hit_radius");

		ClassDB::bind_method(D_METHOD("get_root_bones"), &VRMSpringBone::get_root_bones);
		ClassDB::bind_method(D_METHOD("set_root_bones", "value"), &VRMSpringBone::set_root_bones);
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "root_bones", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_root_bones", "get_root_bones");

		ClassDB::bind_method(D_METHOD("get_collider_groups"), &VRMSpringBone::get_collider_groups);
		ClassDB::bind_method(D_METHOD("set_collider_groups", "value"), &VRMSpringBone::set_collider_groups);
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collider_groups", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_collider_groups", "get_collider_groups");
	}

public:
	void setup(bool force = false) {
		if (!root_bones.is_empty() && skel != nullptr) {
			if (force || verlets.is_empty()) {
				if (!verlets.is_empty()) {
					for (int i = 0; i < verlets.size(); ++i) {
						VRMSpringBoneLogic *verlet = Object::cast_to<VRMSpringBoneLogic>(verlets[i]);
						if (verlet) {
							verlet->reset(skel);
						}
					}
				}
				verlets.clear();
				for (int i = 0; i < root_bones.size(); ++i) {
					String bone_name = root_bones[i];
					if (!bone_name.is_empty()) {
						setup_recursive(skel->find_bone(bone_name), Transform3D());
					}
				}
			}
		}
	}

	void _process(float delta) {
		if (verlets.is_empty()) {
			if (root_bones.is_empty()) {
				return;
			}
			setup();
		}

		float stiffness = stiffness_force * delta;
		Vector3 external = gravity_dir * (gravity_power * delta);

		for (int32_t verlet_i = 0; verlet_i < verlets.size(); verlet_i++) {
			Ref<VRMSpringBoneLogic> verlet = verlets[verlet_i];
			verlet->radius = hit_radius;
			verlet->update(skel, center, stiffness, drag_force, external, colliders);
		}
	}

	void setup_recursive(int id, Transform3D center_tr) {
		Vector<int> bone_children = skel->get_bone_children(id);
		if (bone_children.is_empty()) {
			Vector3 delta = skel->get_bone_rest(id).origin;
			Vector3 child_position = delta.normalized() * 0.07f;
			Ref<VRMSpringBoneLogic> spring_bone_logic = memnew(VRMSpringBoneLogic(skel, id, center_tr.origin, child_position, skel->get_bone_global_pose_no_override(id)));
			verlets.append(spring_bone_logic);
		} else {
			int first_child = bone_children[0];
			Vector3 local_position = skel->get_bone_rest(first_child).origin;
			Vector3 sca = skel->get_bone_rest(first_child).basis.get_scale();
			Vector3 pos(local_position.x * sca.x, local_position.y * sca.y, local_position.z * sca.z);
			Ref<VRMSpringBoneLogic> spring_bone_logic = memnew(VRMSpringBoneLogic(skel, id, center_tr.origin, pos, skel->get_bone_global_pose_no_override(id)));
			verlets.append(spring_bone_logic);
		}
		for (int i = 0; i < bone_children.size(); ++i) {
			setup_recursive(bone_children[i], center_tr);
		}
	}

	void _ready(Skeleton3D *ready_skel, Array colliders_ref) {
		if (ready_skel == nullptr) {
			return;
		}
		setup();
		colliders = colliders_ref.duplicate(false);
	}
};

#endif // VRMSPRINGBONE_H
#pragma once

#ifdef GDJ_CONFIG_EDITOR

#include "joints/jolt_joint_gizmo_plugin_3d.hpp"

class JoltEditorPlugin final : public EditorPlugin {
	GDCLASS_NO_WARN(JoltEditorPlugin, EditorPlugin)

private:
	static void _bind_methods() { }

public:
	void _enter_tree() GDEX_OVERRIDE_EX_ONLY;

	void _exit_tree() GDEX_OVERRIDE_EX_ONLY;
protected:
#ifndef GDEXTENSION
	void _notification(int p_what) {
		switch(p_what) {
			case NOTIFICATION_ENTER_TREE: {
				_enter_tree();
			} break;
			case NOTIFICATION_EXIT_TREE: {
				_exit_tree();
			} break;
		}
	}
#endif

private:
	Ref<JoltJointGizmoPlugin3D> joint_gizmo_plugin;
};

#endif // GDJ_CONFIG_EDITOR

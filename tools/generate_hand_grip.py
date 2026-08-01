import bpy
import math
import os
import sys


def args_after_separator():
    if "--" not in sys.argv:
        raise RuntimeError(
            "Usage: blender --background --python generate_hand_grip_fixed.py -- input.glb output.glb"
        )
    return sys.argv[sys.argv.index("--") + 1:]


def import_model(filepath):
    ext = os.path.splitext(filepath)[1].lower()
    if ext in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=filepath)
    elif ext == ".fbx":
        bpy.ops.import_scene.fbx(filepath=filepath)
    else:
        raise RuntimeError(f"Unsupported input format: {ext}. Use GLB, glTF, or FBX.")


def find_armature():
    armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    if not armatures:
        raise RuntimeError("No armature was found in the imported model")
    # Prefer the armature with the largest number of bones.
    return max(armatures, key=lambda obj: len(obj.data.bones))


def set_rotation(armature, name, x=0.0, y=0.0, z=0.0, required=True):
    bone = armature.pose.bones.get(name)
    if bone is None:
        if required:
            raise RuntimeError(f"Missing required hand bone: {name}")
        return None

    bone.rotation_mode = "XYZ"
    bone.rotation_euler = (
        math.radians(x),
        math.radians(y),
        math.radians(z),
    )
    return bone


def first_existing_bone(armature, names):
    for name in names:
        if armature.pose.bones.get(name) is not None:
            return name
    return None


args = args_after_separator()
if len(args) < 2:
    raise RuntimeError("Expected input model and output GLB paths after --")

source_path = os.path.abspath(args[0])
output_path = os.path.abspath(args[1])

bpy.ops.wm.read_factory_settings(use_empty=True)
import_model(source_path)

armature = find_armature()
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if not meshes:
    raise RuntimeError("The imported model does not contain a mesh")

# Prevent old actions/NLA strips from affecting the authored grip pose.
armature.animation_data_create()
for track in armature.animation_data.nla_tracks:
    track.mute = True
armature.animation_data.action = None

bpy.ops.object.select_all(action="DESELECT")
armature.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.object.mode_set(mode="POSE")
bpy.ops.pose.select_all(action="SELECT")
bpy.ops.pose.transforms_clear()

old_action = bpy.data.actions.get("PistolGrip")
if old_action is not None:
    bpy.data.actions.remove(old_action)

action = bpy.data.actions.new("PistolGrip")
armature.animation_data.action = action

posed_bones = []

# The previous pose did not curl the gripping fingers far enough around the
# narrow P-38 grip. These angles form a tighter cylinder around the handle.
# Local X is the principal bend axis on this hand rig.
for result in [
    # Trigger finger: mostly straight, with a small natural bend.
    set_rotation(armature, "finger_index.01.R", x=8, y=-2, z=-5),
    set_rotation(armature, "finger_index.02.R", x=14, z=-2),
    set_rotation(armature, "finger_index.03.R", x=10),

    # Close the three lower fingers firmly around the narrow grip. Keep their
    # sideways spread subtle so the fingertips travel behind the front strap
    # instead of opening away from it.
    set_rotation(armature, "finger_middle.01.R", x=78, y=-3, z=-7),
    set_rotation(armature, "finger_middle.02.R", x=112),
    set_rotation(armature, "finger_middle.03.R", x=88),

    set_rotation(armature, "finger_ring.01.R", x=84, y=-1, z=-3),
    set_rotation(armature, "finger_ring.02.R", x=116),
    set_rotation(armature, "finger_ring.03.R", x=92),

    set_rotation(armature, "finger_pinky.01.R", x=90, y=2, z=5),
    set_rotation(armature, "finger_pinky.02.R", x=120),
    set_rotation(armature, "finger_pinky.03.R", x=96),

    # Push the thumb across the back/side of the grip and curl the tip inward.
    set_rotation(armature, "thumb.01.R", x=16, y=-5, z=-32),
    set_rotation(armature, "thumb.02.R", x=34, y=-3, z=-10),
    set_rotation(armature, "thumb.03.R", x=28, z=-4),
]:
    if result is not None:
        posed_bones.append(result)

# Some hand rigs include metacarpal/palm bones. Slightly cupping them improves
# contact with the grip, but they are optional so the script remains compatible.
optional_palm_rotations = {
    "palm.01.R": (5, 0, -2),
    "palm.02.R": (7, 0, 0),
    "palm.03.R": (9, 0, 2),
    "palm.04.R": (11, 0, 4),
    "metacarpal_index.R": (4, 0, -2),
    "metacarpal_middle.R": (6, 0, 0),
    "metacarpal_ring.R": (8, 0, 2),
    "metacarpal_pinky.R": (10, 0, 4),
}
for bone_name, rotation in optional_palm_rotations.items():
    bone = set_rotation(
        armature,
        bone_name,
        x=rotation[0],
        y=rotation[1],
        z=rotation[2],
        required=False,
    )
    if bone is not None:
        posed_bones.append(bone)

# Add a very small wrist cant when a conventional wrist/hand control exists.
# This helps the palm follow the backstrap instead of appearing flat.
wrist_name = first_existing_bone(
    armature,
    ["hand.R", "wrist.R", "Hand.R", "RightHand", "mixamorig:RightHand"],
)
if wrist_name:
    wrist = set_rotation(armature, wrist_name, x=-3, y=2, z=-4, required=False)
    if wrist is not None:
        posed_bones.append(wrist)

# Two identical keyed frames create a static animation clip with duration.
for frame in (1, 2):
    bpy.context.scene.frame_set(frame)
    for bone in posed_bones:
        bone.keyframe_insert(
            data_path="rotation_euler",
            frame=frame,
            group=bone.name,
        )

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 2
bpy.context.scene.frame_set(2)
bpy.ops.object.mode_set(mode="OBJECT")

# Export all meshes plus the selected armature. This preserves the pistol if it
# is already part of the source scene as a separate mesh.
bpy.ops.object.select_all(action="DESELECT")
armature.select_set(True)
for mesh in meshes:
    mesh.select_set(True)
bpy.context.view_layer.objects.active = armature

os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
bpy.ops.export_scene.gltf(
    filepath=output_path,
    export_format="GLB",
    use_selection=True,
    export_skins=True,
    export_animations=True,
    export_animation_mode="ACTIONS",
    export_frame_range=True,
    export_force_sampling=True,
)

print(f"Generated {output_path} with tighter action {action.name}")

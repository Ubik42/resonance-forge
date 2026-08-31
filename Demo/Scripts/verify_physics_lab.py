import json
import os
import unreal


MAP_PATH = "/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab"
SHARED_PROFILE_PATH = "/Game/ResonanceForge/Profiles/DA_RF_LongTailWoodString"
REPORT_PATH = os.path.join(unreal.Paths.project_saved_dir(), "ResonanceForge", "physics_lab_verification.json")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"演示地图不存在：{MAP_PATH}")
if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"无法重新加载演示地图：{MAP_PATH}")

balls = []
waveguides = []
waveguide_profile = None
for actor in actor_subsystem.get_all_level_actors():
    label = actor.get_actor_label()
    if label == "RF_04_数字波导弦":
        waveguides.append(label)
        waveguide_profile = actor.native_synth.get_editor_property("material_profile")
    if not label.startswith("RF_落球_"):
        continue
    component = actor.static_mesh_component
    balls.append({
        "label": label,
        "location_z": actor.get_actor_location().z,
        "simulate_physics": component.is_simulating_physics(),
        "gravity_enabled": component.is_gravity_enabled(),
        "mobility": str(component.get_editor_property("mobility")),
        "collision_profile": str(component.get_collision_profile_name()),
        "mass_kg": component.get_mass(),
    })

balls.sort(key=lambda item: item["label"])
failures = []
if len(balls) != 3:
    failures.append(f"预期 3 个落球，实际找到 {len(balls)} 个")
if len(waveguides) != 1:
    failures.append(f"预期 1 个数字波导弦 Actor，实际找到 {len(waveguides)} 个")
if waveguide_profile is None:
    failures.append("数字波导弦没有挂载共享配方")
elif waveguide_profile.get_path_name().split(".")[0] != SHARED_PROFILE_PATH:
    failures.append(f"数字波导弦共享配方错误：{waveguide_profile.get_path_name()}")
for ball in balls:
    if not ball["simulate_physics"]:
        failures.append(f"{ball['label']} 未启用物理模拟")
    if not ball["gravity_enabled"]:
        failures.append(f"{ball['label']} 未启用重力")
    if "MOVABLE" not in ball["mobility"].upper():
        failures.append(f"{ball['label']} Mobility 不是 Movable：{ball['mobility']}")
    if ball["collision_profile"] != "PhysicsActor":
        failures.append(f"{ball['label']} 碰撞预设不是 PhysicsActor：{ball['collision_profile']}")
    if ball["mass_kg"] <= 0.0:
        failures.append(f"{ball['label']} 质量无效：{ball['mass_kg']}")

report = {
    "map": MAP_PATH,
    "verification_scope": "重新加载磁盘地图后的组件状态",
    "ball_count": len(balls),
    "waveguide_actors": waveguides,
    "waveguide_profile": waveguide_profile.get_path_name() if waveguide_profile else None,
    "waveguide_profile_parameters": {
        "string_decay": waveguide_profile.get_editor_property("string_decay"),
        "string_damping": waveguide_profile.get_editor_property("string_damping"),
        "body_coupling": waveguide_profile.get_editor_property("body_coupling"),
        "pickup_position": waveguide_profile.get_editor_property("pickup_position"),
    } if waveguide_profile else None,
    "balls": balls,
    "failures": failures,
    "status": "success" if not failures else "failed",
}
os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)

if failures:
    raise RuntimeError("落球后台复检失败：" + "；".join(failures))

unreal.log("RESONANCE_FORGE_PHYSICS_LAB_VERIFIED " + json.dumps(report, ensure_ascii=False))

import json
import os
import unreal


BASE_MAP = "/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab"
OPTIONAL_MAP = "/Game/CarpentersWorkshop/ResonanceForge/L_RF_WorkshopShowcase"
SHARED_PROFILE_PATH = "/Game/ResonanceForge/Profiles/DA_RF_LongTailWoodString"
REPORT_PATH = os.path.join(
    unreal.Paths.project_saved_dir(),
    "ResonanceForge",
    "workshop_showcase_generation.json",
)

asset_library = unreal.EditorAssetLibrary
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
shared_waveguide_profile = unreal.load_asset(SHARED_PROFILE_PATH)
if shared_waveguide_profile is None:
    raise RuntimeError(f"Fab 地图缺少共享配方：{SHARED_PROFILE_PATH}")


def asset(path):
    value = unreal.load_asset(path)
    if value is None:
        raise RuntimeError(f"Fab 展示资产缺失：{path}")
    return value


def spawn_prop(label, path, location, rotation=None, scale=None):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        location,
        rotation or unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(asset(path))
    actor.static_mesh_component.set_collision_profile_name("BlockAll")
    if scale:
        actor.set_actor_scale3d(scale)
    return actor


if not asset_library.does_asset_exist(BASE_MAP):
    raise RuntimeError(f"基础声学工坊不存在：{BASE_MAP}")

asset_library.make_directory("/Game/CarpentersWorkshop/ResonanceForge")
if asset_library.does_asset_exist(OPTIONAL_MAP):
    if not level_subsystem.load_level(OPTIONAL_MAP):
        raise RuntimeError("无法加载已有 Fab 增强展示地图")
else:
    # UE 5.8 对 UWorld 的普通 duplicate_asset 有额外生命周期检查；关卡子系统的
    # 模板复制会关闭当前关卡、创建副本、保存并加载，是官方支持的首次派生路径。
    if not level_subsystem.new_level_from_template(OPTIONAL_MAP, BASE_MAP):
        raise RuntimeError("无法从基础声学工坊创建 Fab 增强展示地图")

# 用真实木工台替换基础几何台面，同时保留所有可公开复现的功能 Actor。
text_overrides = {
    "RF_标签_拉丝钢": ("BRUSHED STEEL", 20),
    "RF_说明_拉丝钢": ("DENSE HIGHS · LONG DECAY\nIMPULSE > EXCITATION ENERGY", 10),
    "RF_标签_硬木": ("HARDWOOD", 20),
    "RF_说明_硬木": ("WARM LOWS · FAST DAMPING\nVELOCITY > BRIGHTNESS", 10),
    "RF_标签_薄玻璃": ("THIN GLASS", 20),
    "RF_说明_薄玻璃": ("SPARSE HIGHS · BRITTLE TAIL\nSIZE > RESONANCE SCALE", 10),
    "RF_波导标题": ("02  DIGITAL WAVEGUIDE STRING", 24),
    "RF_波导说明": ("DELAY-LINE PROPAGATION · DAMPING FEEDBACK · MIDI PITCH + VELOCITY", 11),
    "RF_Wwise标题": ("03  WWISE OUTPUT", 24),
    "RF_Wwise参数": ("ENERGY  RF_ImpactEnergy     BRIGHTNESS  RF_ImpactBrightness     SIZE  RF_ObjectSize", 9),
    "RF_碰撞区标题": ("01  MATERIAL IMPACT BENCH", 24),
    "RF_标题": ("RESONANCE FORGE · ACOUSTIC WORKSHOP", 32),
    "RF_副标题": ("SELECT OBJECT > EXCITE > RESONATE > PUBLISH TO WWISE", 13),
}
text_layout = {
    "RF_编号_1": (unreal.Vector(-530, -132, 255), 18),
    "RF_标签_拉丝钢": (unreal.Vector(-570, -132, 250), 20),
    "RF_说明_拉丝钢": (unreal.Vector(-530, -131, 205), 10),
    "RF_编号_2": (unreal.Vector(-220, -132, 255), 18),
    "RF_标签_硬木": (unreal.Vector(-260, -132, 250), 20),
    "RF_说明_硬木": (unreal.Vector(-220, -131, 205), 10),
    "RF_编号_3": (unreal.Vector(90, -132, 255), 18),
    "RF_标签_薄玻璃": (unreal.Vector(50, -132, 250), 20),
    "RF_说明_薄玻璃": (unreal.Vector(90, -131, 205), 10),
    "RF_波导标题": (unreal.Vector(690, -132, 342), 24),
    "RF_波导说明": (unreal.Vector(690, -131, 307), 11),
    "RF_Wwise标题": (unreal.Vector(280, 504, 375), 24),
    "RF_Wwise事件": (unreal.Vector(280, 503, 335), 16),
    "RF_Wwise参数": (unreal.Vector(280, 503, 300), 9),
    "RF_碰撞区标题": (unreal.Vector(-250, -132, 342), 24),
    "RF_标题": (unreal.Vector(650, 587, 552), 32),
    "RF_副标题": (unreal.Vector(650, 586, 510), 13),
}
waveguide_profile_path = None
for actor in actor_subsystem.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("RF_基础工作台_") or label.startswith("RF_Fab_"):
        actor_subsystem.destroy_actor(actor)
    elif label == "RF_演示相机":
        actor.set_actor_location(unreal.Vector(0, -1280, 500), False, False)
        actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-6.0, yaw=90.0), False)
    elif label == "RF_玩家起点":
        actor.set_actor_location(unreal.Vector(0, -1280, 450), False, False)
        actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-5.0, yaw=90.0), False)
    elif isinstance(actor, unreal.TextRenderActor):
        actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), False)
        if label in text_layout:
            location, world_size = text_layout[label]
            actor.set_actor_location(location, False, False)
            actor.text_render.set_world_size(world_size)
        if label in text_overrides:
            text, world_size = text_overrides[label]
            actor.text_render.set_text(text)
            actor.text_render.set_world_size(world_size)
    elif label.startswith("RF_材质展灯_"):
        actor.light_component.set_intensity(260.0)
    elif label == "RF_主光":
        actor.light_component.set_intensity(1.55)
    elif label == "RF_04_数字波导弦":
        actor.native_synth.apply_material_profile(shared_waveguide_profile)
        waveguide_profile_path = actor.native_synth.get_editor_property("material_profile").get_path_name()

if not waveguide_profile_path or waveguide_profile_path.split(".")[0] != SHARED_PROFILE_PATH:
    raise RuntimeError(f"Fab 数字弦共享配方挂载失败：{waveguide_profile_path}")

props = [
    spawn_prop(
        "RF_Fab_木工主台",
        "/Game/CarpentersWorkshop/Meshes/SM_Workbench_01",
        unreal.Vector(-365, 30, -27),
        scale=unreal.Vector(1.35, 1.35, 1.35),
    ),
    spawn_prop(
        "RF_Fab_钢锤",
        "/Game/CarpentersWorkshop/Meshes/SM_Hammer_01",
        unreal.Vector(-760, -8, 124),
        unreal.Rotator(roll=12.0, pitch=90.0, yaw=78.0),
        unreal.Vector(1.45, 1.45, 1.45),
    ),
    spawn_prop(
        "RF_Fab_木槌",
        "/Game/CarpentersWorkshop/Meshes/SM_Mallet_01",
        unreal.Vector(-480, 78, 126),
        unreal.Rotator(roll=-18.0, pitch=84.0, yaw=-72.0),
        unreal.Vector(1.65, 1.65, 1.65),
    ),
    spawn_prop(
        "RF_Fab_材料木板",
        "/Game/CarpentersWorkshop/Meshes/SM_Plank_01",
        unreal.Vector(-270, 92, 124),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=12.0),
        unreal.Vector(1.05, 1.05, 1.05),
    ),
    spawn_prop(
        "RF_Fab_材料钢板",
        "/Game/CarpentersWorkshop/Meshes/SM_Metal_Plate",
        unreal.Vector(-78, 72, 132),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-9.0),
        unreal.Vector(1.7, 1.7, 1.7),
    ),
    spawn_prop(
        "RF_Fab_工具箱",
        "/Game/CarpentersWorkshop/Meshes/SM_Wood_Toolbox",
        unreal.Vector(930, 135, -27),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-18.0),
        unreal.Vector(1.18, 1.18, 1.18),
    ),
]

if not level_subsystem.save_current_level():
    raise RuntimeError("Fab 增强展示地图保存失败")

report = {
    "base_map": BASE_MAP,
    "optional_map": OPTIONAL_MAP,
    "fab_package": "Carpenter's Workshop Environment",
    "prop_count": len(props),
    "props": [actor.get_actor_label() for actor in props],
    "public_repository_dependency": False,
    "shared_profile": waveguide_profile_path,
    "status": "success",
}
os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)

unreal.log("RESONANCE_FORGE_WORKSHOP_SHOWCASE_READY " + json.dumps(report, ensure_ascii=False))

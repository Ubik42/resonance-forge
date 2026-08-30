import json
import os
import unreal


MAP_PATH = "/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab"
REPORT_PATH = os.path.join(unreal.Paths.project_saved_dir(), "ResonanceForge", "physics_lab_generation.json")


def set_prop(obj, name, value):
    obj.set_editor_property(name, value)


def asset(path):
    value = unreal.load_asset(path)
    if value is None:
        raise RuntimeError(f"演示资产缺失：{path}")
    return value


asset_library = unreal.EditorAssetLibrary
asset_library.make_directory("/Game/ResonanceForge/Demo/Maps")
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if asset_library.does_asset_exist(MAP_PATH):
    level_subsystem.load_level(MAP_PATH)
    for actor in actor_subsystem.get_all_level_actors():
        actor_subsystem.destroy_actor(actor)
else:
    if not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError("无法创建共振铸造台演示地图")

cube_mesh = asset("/Engine/BasicShapes/Cube.Cube")
sphere_mesh = asset("/Engine/BasicShapes/Sphere.Sphere")
materials = {
    "Steel": asset("/Game/ResonanceForge/Demo/Materials/MI_RF_Steel.MI_RF_Steel"),
    "Wood": asset("/Game/ResonanceForge/Demo/Materials/MI_RF_Wood.MI_RF_Wood"),
    "Glass": asset("/Game/ResonanceForge/Demo/Materials/MI_RF_Glass.MI_RF_Glass"),
    "Floor": asset("/Game/ResonanceForge/Demo/Materials/M_RF_Floor.M_RF_Floor"),
    "Dark": asset("/Game/ResonanceForge/Demo/Materials/M_RF_Dark.M_RF_Dark"),
    "SteelAccent": asset("/Game/ResonanceForge/Demo/Materials/M_RF_AccentSteel.M_RF_AccentSteel"),
    "WoodAccent": asset("/Game/ResonanceForge/Demo/Materials/M_RF_AccentWood.M_RF_AccentWood"),
    "GlassAccent": asset("/Game/ResonanceForge/Demo/Materials/M_RF_AccentGlass.M_RF_AccentGlass"),
    "Wwise": asset("/Game/ResonanceForge/Demo/Materials/M_RF_WwiseCore.M_RF_WwiseCore"),
}


def spawn_mesh(label, location, scale, mesh=cube_mesh, material=None, simulate=False, rotation=None):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor, location, rotation or unreal.Rotator(0.0, 0.0, 0.0)
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(scale)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_collision_profile_name("PhysicsActor" if simulate else "BlockAll")
    if material:
        component.set_material(0, material)
    if simulate:
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_enable_gravity(True)
        component.set_notify_rigid_body_collision(True)
        component.set_simulate_physics(True)
        component.set_mass_override_in_kg("None", 18.0, True)
    return actor


def spawn_text(label, text, location, size, color):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.TextRenderActor,
        location,
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0),
    )
    actor.set_actor_label(label)
    component = actor.text_render
    component.set_text(text)
    component.set_world_size(size)
    component.set_text_render_color(color)
    return actor


# 小型声学工坊：左侧做材质碰撞，右侧做数字波导弦，后墙汇入 Wwise。
spawn_mesh("RF_工坊地面", unreal.Vector(0, 80, -72), unreal.Vector(13.5, 8.7, 0.45), material=materials["Floor"])
spawn_mesh("RF_工坊后墙", unreal.Vector(0, 610, 285), unreal.Vector(13.5, 0.20, 3.65), material=materials["Dark"])
spawn_mesh("RF_顶部声学轨", unreal.Vector(0, 565, 615), unreal.Vector(13.5, 0.18, 0.055), material=materials["Wwise"])

# 无第三方包时也能成立的基础工作台；Fab 增强地图会用真实木工台替换它。
spawn_mesh("RF_基础工作台_台面", unreal.Vector(-365, 30, 72), unreal.Vector(4.9, 2.25, 0.18), material=materials["Wood"])
spawn_mesh("RF_基础工作台_左腿", unreal.Vector(-760, 30, -2), unreal.Vector(0.18, 1.85, 0.75), material=materials["Dark"])
spawn_mesh("RF_基础工作台_右腿", unreal.Vector(30, 30, -2), unreal.Vector(0.18, 1.85, 0.75), material=materials["Dark"])

instrument_specs = [
    {
        "label": "RF_01_拉丝钢", "preset": "拉丝钢", "key": "Steel", "accent": "SteelAccent",
        "location": unreal.Vector(-675, 10, 126), "size": 0.36, "ball_z": 465,
        "display": "BRUSHED STEEL", "color": unreal.Color(57, 166, 255, 255),
        "description": "DENSE HIGHS · LONG DECAY\nIMPULSE > EXCITATION ENERGY",
    },
    {
        "label": "RF_02_硬木", "preset": "硬木", "key": "Wood", "accent": "WoodAccent",
        "location": unreal.Vector(-365, 10, 126), "size": 0.58, "ball_z": 555,
        "display": "HARDWOOD", "color": unreal.Color(255, 112, 39, 255),
        "description": "WARM LOWS · FAST DAMPING\nVELOCITY > BRIGHTNESS",
    },
    {
        "label": "RF_03_薄玻璃", "preset": "薄玻璃", "key": "Glass", "accent": "GlassAccent",
        "location": unreal.Vector(-55, 10, 126), "size": 0.24, "ball_z": 645,
        "display": "THIN GLASS", "color": unreal.Color(58, 245, 205, 255),
        "description": "SPARSE HIGHS · BRITTLE TAIL\nSIZE > RESONANCE SCALE",
    },
]

instruments = []
balls = []
for index, spec in enumerate(instrument_specs):
    location = spec["location"]

    # 每种材质都有独立的撞击砧座，并通过后方信号轨连接输出区。
    spawn_mesh(f"RF_砧座_{index + 1}", unreal.Vector(location.x, 10, 104), unreal.Vector(1.30, 1.32, 0.12), material=materials["Dark"])
    spawn_mesh(f"RF_导光条_{index + 1}", unreal.Vector(location.x, -58, 108), unreal.Vector(1.18, 0.035, 0.035), material=materials[spec["accent"]])
    spawn_mesh(f"RF_信号轨_{index + 1}", unreal.Vector(location.x, 330, 88), unreal.Vector(0.025, 3.18, 0.025), material=materials[spec["accent"]])

    instrument = actor_subsystem.spawn_actor_from_class(unreal.ResonanceForgeImpactInstrumentActor, location)
    instrument.set_actor_label(spec["label"])
    instrument.set_actor_scale3d(unreal.Vector(1.16, 1.16, 0.20))
    set_prop(instrument, "resonance_preset", unreal.Name(spec["preset"]))
    set_prop(instrument, "object_size", spec["size"])
    set_prop(instrument, "minimum_impulse", 650.0)
    set_prop(instrument, "impulse_sensitivity", 0.00012)
    set_prop(instrument, "enable_keyboard_trigger", index == 0)
    instrument.instrument_mesh.set_static_mesh(cube_mesh)
    instrument.instrument_mesh.set_material(0, materials[spec["key"]])
    instrument.instrument_mesh.set_collision_profile_name("BlockAll")
    instrument.instrument_mesh.set_notify_rigid_body_collision(True)
    instruments.append(instrument)

    ball = spawn_mesh(
        f"RF_落球_{index + 1}", unreal.Vector(location.x, 0, spec["ball_z"]),
        unreal.Vector(0.68, 0.68, 0.68), sphere_mesh, materials[spec["key"]], True
    )
    balls.append(ball)

    spawn_text(f"RF_编号_{index + 1}", f"0{index + 1}", unreal.Vector(location.x + 145, -132, 255), 18, spec["color"])
    spawn_text(f"RF_标签_{spec['preset']}", spec["display"], unreal.Vector(location.x + 105, -132, 250), 20, unreal.Color(235, 244, 252, 255))
    spawn_text(f"RF_说明_{spec['preset']}", spec["description"], unreal.Vector(location.x + 145, -131, 205), 10, spec["color"])

    point = actor_subsystem.spawn_actor_from_class(unreal.PointLight, unreal.Vector(location.x, -25, 315))
    point.set_actor_label(f"RF_材质展灯_{index + 1}")
    point.light_component.set_intensity(260.0)
    point.light_component.set_attenuation_radius(390.0)
    point.light_component.set_light_color(unreal.LinearColor(
        spec["color"].r / 255.0, spec["color"].g / 255.0, spec["color"].b / 255.0, 1.0
    ))
    point.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

# 右侧数字波导弦架：一个真正的可演奏 Actor，加五条可见弦线表达模型结构。
spawn_mesh("RF_波导弦底座", unreal.Vector(500, 40, 54), unreal.Vector(4.65, 2.20, 0.18), material=materials["Dark"])
spawn_mesh("RF_波导弦左桥", unreal.Vector(170, 40, 174), unreal.Vector(0.12, 0.38, 1.20), material=materials["Steel"])
spawn_mesh("RF_波导弦右桥", unreal.Vector(830, 40, 174), unreal.Vector(0.12, 0.38, 1.20), material=materials["Steel"])
for string_index, note_name in enumerate(["C3", "E3", "G3", "B3", "E4"]):
    string_z = 112 + string_index * 34
    spawn_mesh(
        f"RF_可视弦_{note_name}", unreal.Vector(500, 40, string_z),
        unreal.Vector(3.28, 0.018, 0.018), material=materials["WoodAccent"]
    )
    spawn_text(
        f"RF_弦音高_{note_name}", note_name, unreal.Vector(858, -6, string_z + 7),
        18, unreal.Color(255, 137, 68, 255)
    )

waveguide = actor_subsystem.spawn_actor_from_class(
    unreal.ResonanceForgeImpactInstrumentActor, unreal.Vector(500, 42, 174)
)
waveguide.set_actor_label("RF_04_数字波导弦")
waveguide.set_actor_scale3d(unreal.Vector(3.25, 0.20, 0.05))
set_prop(waveguide, "resonance_preset", unreal.Name("硬木"))
set_prop(waveguide, "synthesis_model", unreal.ResonanceModelType.WAVEGUIDE_STRING)
set_prop(waveguide, "object_size", 0.46)
set_prop(waveguide, "enable_keyboard_trigger", False)
waveguide.instrument_mesh.set_static_mesh(cube_mesh)
waveguide.instrument_mesh.set_material(0, materials["WoodAccent"])

spawn_text("RF_波导标题", "02  DIGITAL WAVEGUIDE STRING", unreal.Vector(690, -132, 342), 24, unreal.Color(255, 149, 75, 255))
spawn_text("RF_波导说明", "DELAY-LINE PROPAGATION · DAMPING FEEDBACK · MIDI PITCH + VELOCITY", unreal.Vector(690, -131, 307), 11, unreal.Color(235, 225, 214, 255))

# 两类声源在后墙汇入同一组 Wwise Event / RTPC。
spawn_mesh("RF_Wwise核心", unreal.Vector(285, 545, 318), unreal.Vector(6.15, 0.24, 0.72), material=materials["Wwise"])
spawn_text("RF_Wwise标题", "03  WWISE OUTPUT", unreal.Vector(280, 504, 375), 24, unreal.Color(86, 226, 255, 255))
spawn_text("RF_Wwise事件", "Play_RF_Impact_Metal", unreal.Vector(280, 503, 335), 16, unreal.Color(235, 248, 255, 255))
spawn_text("RF_Wwise参数", "ENERGY  RF_ImpactEnergy     BRIGHTNESS  RF_ImpactBrightness     SIZE  RF_ObjectSize", unreal.Vector(280, 503, 300), 9, unreal.Color(126, 218, 238, 255))

spawn_text("RF_碰撞区标题", "01  MATERIAL IMPACT BENCH", unreal.Vector(-250, -132, 342), 24, unreal.Color(82, 211, 255, 255))
spawn_text("RF_标题", "RESONANCE FORGE · ACOUSTIC WORKSHOP", unreal.Vector(650, 587, 552), 32, unreal.Color(235, 247, 255, 255))
spawn_text("RF_副标题", "SELECT OBJECT > EXCITE > RESONATE > PUBLISH TO WWISE", unreal.Vector(650, 586, 510), 13, unreal.Color(102, 222, 255, 255))

directional = actor_subsystem.spawn_actor_from_class(
    unreal.DirectionalLight,
    unreal.Vector(0, 0, 900),
    unreal.Rotator(roll=0.0, pitch=-52.0, yaw=-32.0),
)
directional.set_actor_label("RF_主光")
directional.light_component.set_intensity(1.55)
directional.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

sky = actor_subsystem.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
sky.set_actor_label("RF_环境光")
sky.light_component.set_intensity(0.42)
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

atmosphere = actor_subsystem.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
atmosphere.set_actor_label("RF_天空大气")

camera = actor_subsystem.spawn_actor_from_class(
    unreal.CineCameraActor,
    unreal.Vector(0, -1280, 500),
    unreal.Rotator(roll=0.0, pitch=-6.0, yaw=90.0),
)
camera.set_actor_label("RF_演示相机")
camera.get_cine_camera_component().set_editor_property("current_focal_length", 42.0)

player_start = actor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart,
    unreal.Vector(0, -1280, 450),
    unreal.Rotator(roll=0.0, pitch=-5.0, yaw=90.0),
)
player_start.set_actor_label("RF_玩家起点")

if not level_subsystem.save_current_level():
    raise RuntimeError("共振铸造台演示地图保存失败")

report = {
    "map": MAP_PATH,
    "instrument_count": len(instruments),
    "physics_ball_count": len(balls),
    "presets": [spec["preset"] for spec in instrument_specs],
    "surface_materials": [spec["key"] for spec in instrument_specs],
    "wwise_event": "Play_RF_Impact_Metal",
    "wwise_rtpcs": ["RF_ImpactEnergy", "RF_ImpactBrightness", "RF_ObjectSize"],
    "waveguide_actor": waveguide.get_actor_label(),
    "visual_story": "材质碰撞台与数字波导弦在后墙汇入 Wwise Event / RTPC 输出",
    "physics_balls": [
        {
            "label": ball.get_actor_label(),
            "simulate_physics": ball.static_mesh_component.is_simulating_physics(),
            "mobility": str(ball.static_mesh_component.get_editor_property("mobility")),
            "gravity_enabled": ball.static_mesh_component.is_gravity_enabled(),
            "material": ball.static_mesh_component.get_material(0).get_path_name(),
        }
        for ball in balls
    ],
    "status": "success",
}

invalid_balls = [entry for entry in report["physics_balls"] if not entry["simulate_physics"] or not entry["gravity_enabled"] or "MOVABLE" not in entry["mobility"].upper()]
if invalid_balls:
    raise RuntimeError("落球物理状态复检失败：" + json.dumps(invalid_balls, ensure_ascii=False))
os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)

unreal.log("RESONANCE_FORGE_PHYSICS_LAB_READY " + json.dumps(report, ensure_ascii=False))

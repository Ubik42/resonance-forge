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
        unreal.TextRenderActor, location, unreal.Rotator(90.0, 0.0, 0.0)
    )
    actor.set_actor_label(label)
    component = actor.text_render
    component.set_text(text)
    component.set_world_size(size)
    component.set_text_render_color(color)
    return actor


# 舞台壳层：深色地面、后墙与横向发光导轨。
spawn_mesh("RF_实验室地面", unreal.Vector(0, 50, -72), unreal.Vector(13.5, 8.5, 0.45), material=materials["Floor"])
spawn_mesh("RF_后墙", unreal.Vector(0, 510, 280), unreal.Vector(13.5, 0.20, 3.6), material=materials["Dark"])
spawn_mesh("RF_顶部框架", unreal.Vector(0, 465, 610), unreal.Vector(13.5, 0.28, 0.10), material=materials["Wwise"])

instrument_specs = [
    {
        "label": "RF_01_拉丝钢", "preset": "拉丝钢", "key": "Steel", "accent": "SteelAccent",
        "location": unreal.Vector(-430, 0, 35), "size": 0.36, "ball_z": 570,
        "color": unreal.Color(57, 166, 255, 255), "description": "高频密集 · 长衰减\nImpulse → Energy",
    },
    {
        "label": "RF_02_硬木", "preset": "硬木", "key": "Wood", "accent": "WoodAccent",
        "location": unreal.Vector(0, 0, 35), "size": 0.58, "ball_z": 670,
        "color": unreal.Color(255, 112, 39, 255), "description": "中低频突出 · 快阻尼\nVelocity → Brightness",
    },
    {
        "label": "RF_03_薄玻璃", "preset": "薄玻璃", "key": "Glass", "accent": "GlassAccent",
        "location": unreal.Vector(430, 0, 35), "size": 0.24, "ball_z": 770,
        "color": unreal.Color(58, 245, 205, 255), "description": "稀疏高频 · 脆性尾音\nSize → Resonance",
    },
]

instruments = []
balls = []
for index, spec in enumerate(instrument_specs):
    location = spec["location"]

    # 带编号的独立实验舱与发光边界。
    spawn_mesh(f"RF_底座_{index + 1}", unreal.Vector(location.x, 0, -6), unreal.Vector(3.25, 3.25, 0.22), material=materials["Dark"])
    spawn_mesh(f"RF_导光条_{index + 1}", unreal.Vector(location.x, -162, 2), unreal.Vector(3.10, 0.055, 0.055), material=materials[spec["accent"]])
    spawn_mesh(f"RF_信号轨_{index + 1}", unreal.Vector(location.x, 255, -10), unreal.Vector(0.035, 2.55, 0.035), material=materials[spec["accent"]])

    instrument = actor_subsystem.spawn_actor_from_class(unreal.ResonanceForgeImpactInstrumentActor, location)
    instrument.set_actor_label(spec["label"])
    instrument.set_actor_scale3d(unreal.Vector(2.85, 2.85, 0.34))
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

    spawn_text(f"RF_编号_{index + 1}", f"0{index + 1}", unreal.Vector(location.x - 142, -188, 192), 34, spec["color"])
    spawn_text(f"RF_标签_{spec['preset']}", spec["preset"], unreal.Vector(location.x - 95, -188, 185), 54, unreal.Color(235, 244, 252, 255))
    spawn_text(f"RF_说明_{spec['preset']}", spec["description"], unreal.Vector(location.x - 142, -187, 118), 24, spec["color"])

    point = actor_subsystem.spawn_actor_from_class(unreal.PointLight, unreal.Vector(location.x, -30, 250))
    point.set_actor_label(f"RF_材质展灯_{index + 1}")
    point.light_component.set_intensity(1150.0)
    point.light_component.set_attenuation_radius(520.0)
    point.light_component.set_light_color(unreal.LinearColor(
        spec["color"].r / 255.0, spec["color"].g / 255.0, spec["color"].b / 255.0, 1.0
    ))
    point.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

# 三路材质参数在后墙汇入 Wwise Event 核心。
spawn_mesh("RF_Wwise核心", unreal.Vector(0, 415, 215), unreal.Vector(2.25, 0.30, 0.80), material=materials["Wwise"])
spawn_text("RF_Wwise标题", "WWISE EVENT", unreal.Vector(-138, 372, 266), 36, unreal.Color(86, 226, 255, 255))
spawn_text("RF_Wwise事件", "Play_RF_Impact_Metal", unreal.Vector(-172, 371, 205), 28, unreal.Color(235, 248, 255, 255))
spawn_text("RF_Wwise参数", "RF_ImpactEnergy  ·  RF_ImpactBrightness  ·  RF_ObjectSize", unreal.Vector(-305, 371, 152), 18, unreal.Color(101, 198, 222, 255))

spawn_text("RF_标题", "共振铸造台", unreal.Vector(-250, 487, 535), 72, unreal.Color(235, 247, 255, 255))
spawn_text("RF_副标题", "物理材质 → 模态共振 → MIDI 表演 → Wwise 发布", unreal.Vector(-420, 486, 460), 30, unreal.Color(72, 215, 255, 255))

directional = actor_subsystem.spawn_actor_from_class(
    unreal.DirectionalLight, unreal.Vector(0, 0, 900), unreal.Rotator(-52, -32, 0)
)
directional.set_actor_label("RF_主光")
directional.light_component.set_intensity(3.2)
directional.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

sky = actor_subsystem.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
sky.set_actor_label("RF_环境光")
sky.light_component.set_intensity(0.34)
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

atmosphere = actor_subsystem.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
atmosphere.set_actor_label("RF_天空大气")

camera = actor_subsystem.spawn_actor_from_class(
    unreal.CineCameraActor, unreal.Vector(0, -2050, 760), unreal.Rotator(-12, 90, 0)
)
camera.set_actor_label("RF_演示相机")
camera.get_cine_camera_component().set_editor_property("current_focal_length", 42.0)

player_start = actor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(0, -2050, 650), unreal.Rotator(-10, 90, 0)
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
    "visual_story": "三种视觉材质通过发光信号轨汇入 Wwise Event 核心",
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

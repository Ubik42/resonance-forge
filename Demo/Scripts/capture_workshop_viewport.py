from pathlib import Path

import unreal


BASE_MAP = "/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab"
FAB_MAP = "/Game/CarpentersWorkshop/ResonanceForge/L_RF_WorkshopShowcase"
OUTPUT_PATH = Path(unreal.Paths.project_saved_dir()) / "Screenshots" / "ResonanceForge_Workshop.png"

library = unreal.EditorAssetLibrary
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_map = FAB_MAP if library.does_asset_exist(FAB_MAP) else BASE_MAP

if not level_subsystem.load_level(target_map):
    raise RuntimeError(f"无法加载截图地图：{target_map}")

camera = next(
    (actor for actor in actor_subsystem.get_all_level_actors() if actor.get_actor_label() == "RF_演示相机"),
    None,
)
if camera is None:
    raise RuntimeError("场景中缺少 RF_演示相机")

level_subsystem.set_level_viewport_camera_info(
    camera.get_actor_location(),
    camera.get_actor_rotation(),
    unreal.Name("FourPanes2x2.Viewport 1.Viewport1"),
)
level_subsystem.set_level_viewport_fov(65.0, unreal.Name("FourPanes2x2.Viewport 1.Viewport1"))
OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)

state = {"frames": 0, "captured": False, "handle": None}


def on_tick(_delta_seconds):
    state["frames"] += 1
    world = unreal.EditorLevelLibrary.get_editor_world()
    if state["frames"] == 30:
        unreal.SystemLibrary.execute_console_command(world, "ShowFlag.Editor 0")
        unreal.SystemLibrary.execute_console_command(world, "ShowFlag.Grid 0")
        unreal.SystemLibrary.execute_console_command(world, "ShowFlag.Selection 0")
    if state["frames"] == 45:
        command = f'HighResShot 1920x1080 filename="{OUTPUT_PATH.as_posix()}"'
        unreal.SystemLibrary.execute_console_command(world, command)
        state["captured"] = True
        unreal.log(f"RESONANCE_FORGE_SCREENSHOT_REQUESTED {OUTPUT_PATH}")
    if state["frames"] >= 150:
        unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")


state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
unreal.log(f"RESONANCE_FORGE_SCREENSHOT_READY map={target_map}")

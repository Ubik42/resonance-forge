import json
import os
import unreal


PACKAGE_ROOT = "/Game/CarpentersWorkshop"
EXAMPLE_MAP = f"{PACKAGE_ROOT}/Levels/ExampleLevel"
REPORT_PATH = os.path.join(
    unreal.Paths.project_saved_dir(),
    "ResonanceForge",
    "carpenters_workshop_verification.json",
)

REPRESENTATIVE_ASSETS = {
    "workbench": f"{PACKAGE_ROOT}/Meshes/SM_Workbench_01",
    "hammer": f"{PACKAGE_ROOT}/Meshes/SM_Hammer_01",
    "mallet": f"{PACKAGE_ROOT}/Meshes/SM_Mallet_01",
    "metal_plate": f"{PACKAGE_ROOT}/Meshes/SM_Metal_Plate",
    "wood_plank": f"{PACKAGE_ROOT}/Meshes/SM_Plank_01",
    "toolbox": f"{PACKAGE_ROOT}/Meshes/SM_Wood_Toolbox",
    "master_material": f"{PACKAGE_ROOT}/Materials/MM_Base",
    "example_map": EXAMPLE_MAP,
}


def asset_record(key, path):
    data = unreal.EditorAssetLibrary.find_asset_data(path)
    loaded = unreal.load_asset(path)
    record = {
        "key": key,
        "path": path,
        "exists": unreal.EditorAssetLibrary.does_asset_exist(path),
        "loaded": loaded is not None,
        "class": str(data.asset_class_path) if data and data.is_valid() else "",
    }
    if isinstance(loaded, unreal.StaticMesh):
        try:
            bounds = loaded.get_bounding_box()
            record["bounds_min"] = [bounds.min.x, bounds.min.y, bounds.min.z]
            record["bounds_max"] = [bounds.max.x, bounds.max.y, bounds.max.z]
        except Exception as exc:
            record["bounds_note"] = str(exc)
    return record


records = [asset_record(key, path) for key, path in REPRESENTATIVE_ASSETS.items()]
failures = [entry["path"] for entry in records if not entry["exists"] or not entry["loaded"]]

map_loaded = False
if not failures:
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    map_loaded = level_subsystem.load_level(EXAMPLE_MAP)
    if not map_loaded:
        failures.append(EXAMPLE_MAP + "（示例地图无法加载）")

report = {
    "package": "Carpenter's Workshop Environment",
    "package_root": PACKAGE_ROOT,
    "target_engine": "Unreal Engine 5.8.1",
    "manifest_source_version": "5.2.0-22528730",
    "representative_assets": records,
    "example_map_loaded": map_loaded,
    "failures": failures,
    "status": "success" if not failures else "failed",
}

os.makedirs(os.path.dirname(REPORT_PATH), exist_ok=True)
with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)

if failures:
    raise RuntimeError("Carpenter's Workshop 代表资产验证失败：" + "；".join(failures))

unreal.log("RESONANCE_FORGE_CARPENTER_PACKAGE_VERIFIED " + json.dumps(report, ensure_ascii=False))

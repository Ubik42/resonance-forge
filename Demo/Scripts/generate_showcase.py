from pathlib import Path
import unreal


scripts = Path(unreal.Paths.project_dir()) / "Scripts"
required_visual_assets = (
    "/Game/ResonanceForge/Demo/Materials/MI_RF_Steel",
    "/Game/ResonanceForge/Demo/Materials/MI_RF_Wood",
    "/Game/ResonanceForge/Demo/Materials/MI_RF_Glass",
    "/Game/ResonanceForge/Demo/Materials/M_RF_WwiseCore",
)
scripts_to_run = []
if not all(unreal.EditorAssetLibrary.does_asset_exist(path) for path in required_visual_assets):
    scripts_to_run.append("generate_visual_assets.py")
scripts_to_run.append("generate_physics_lab.py")

for script_name in scripts_to_run:
    script_path = scripts / script_name
    code = compile(script_path.read_text(encoding="utf-8"), str(script_path), "exec")
    exec(code, {"__name__": "__main__", "__file__": str(script_path)})

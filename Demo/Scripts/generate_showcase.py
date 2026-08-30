from pathlib import Path
import unreal


scripts = Path(unreal.Paths.project_dir()) / "Scripts"
for script_name in ("generate_visual_assets.py", "generate_physics_lab.py"):
    script_path = scripts / script_name
    code = compile(script_path.read_text(encoding="utf-8"), str(script_path), "exec")
    exec(code, {"__name__": "__main__", "__file__": str(script_path)})

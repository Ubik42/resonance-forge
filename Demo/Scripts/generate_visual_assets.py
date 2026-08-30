import json
import os
from pathlib import Path

import unreal


SOURCE_ROOT = Path(unreal.Paths.project_dir()) / "TestMaterials" / "Generated"
DESTINATION = "/Game/ResonanceForge/Demo/Materials"
REPORT_PATH = Path(unreal.Paths.project_saved_dir()) / "ResonanceForge" / "visual_assets_generation.json"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
library.make_directory(DESTINATION)


def import_texture(source_name, asset_name, srgb):
    object_path = f"{DESTINATION}/{asset_name}.{asset_name}"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(SOURCE_ROOT / source_name))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    texture = library.load_asset(object_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"纹理导入失败：{source_name}")
    texture.set_editor_property("srgb", srgb)
    if not srgb:
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    library.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def get_or_create_material(name):
    path = f"{DESTINATION}/{name}.{name}"
    material = library.load_asset(path) if library.does_asset_exist(path) else None
    if material is None:
        material = asset_tools.create_asset(name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"材质创建失败：{name}")
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    return material


def constant(material, value, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def build_surface_material(key, base_texture, roughness_texture, metallic, translucent=False):
    material = get_or_create_material(f"M_RF_{key}")
    material.set_editor_property("two_sided", translucent)
    if translucent:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    base = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -420, -140)
    base.set_editor_property("texture", base_texture)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -420, 40)
    roughness.set_editor_property("texture", roughness_texture)
    metal = constant(material, metallic, -420, 210)

    if not unreal.MaterialEditingLibrary.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR):
        raise RuntimeError(f"{key} BaseColor 接线失败")
    if not unreal.MaterialEditingLibrary.connect_material_property(roughness, "R", unreal.MaterialProperty.MP_ROUGHNESS):
        raise RuntimeError(f"{key} Roughness 接线失败")
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    if translucent:
        opacity = constant(material, 0.58, -420, 330)
        unreal.MaterialEditingLibrary.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.recompile_material(material)
    library.save_loaded_asset(material, only_if_is_dirty=False)

    instance_name = f"MI_RF_{key}"
    instance_path = f"{DESTINATION}/{instance_name}.{instance_name}"
    instance = library.load_asset(instance_path) if library.does_asset_exist(instance_path) else None
    if instance is None:
        instance = asset_tools.create_asset(instance_name, DESTINATION, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, material)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    library.save_loaded_asset(instance, only_if_is_dirty=False)
    return instance_path


def build_color_material(name, color, metallic=0.0, roughness=0.45, emissive=0.0):
    material = get_or_create_material(name)
    color_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -360, -80)
    color_node.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    metal_node = constant(material, metallic, -360, 90)
    rough_node = constant(material, roughness, -360, 170)
    unreal.MaterialEditingLibrary.connect_material_property(color_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive > 0.0:
        multiply = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -80, 270)
        strength = constant(material, emissive, -360, 310)
        unreal.MaterialEditingLibrary.connect_material_expressions(color_node, "", multiply, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(strength, "", multiply, "B")
        unreal.MaterialEditingLibrary.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    library.save_loaded_asset(material, only_if_is_dirty=False)
    return f"{DESTINATION}/{name}.{name}"


texture_specs = {
    "Steel": ("T_RF_Steel_BaseColor.png", "T_RF_Steel_Roughness.png", 1.0, False),
    "Wood": ("T_RF_Wood_BaseColor.png", "T_RF_Wood_Roughness.png", 0.0, False),
    "Glass": ("T_RF_Glass_BaseColor.png", "T_RF_Glass_Roughness.png", 0.05, True),
}

surface_paths = {}
for key, (base_source, rough_source, metallic, translucent) in texture_specs.items():
    base = import_texture(base_source, f"T_RF_{key}_BaseColor", True)
    roughness = import_texture(rough_source, f"T_RF_{key}_Roughness", False)
    surface_paths[key] = build_surface_material(key, base, roughness, metallic, translucent)

support_paths = {
    "Floor": build_color_material("M_RF_Floor", (0.012, 0.022, 0.038), 0.72, 0.27),
    "Dark": build_color_material("M_RF_Dark", (0.018, 0.034, 0.055), 0.58, 0.34),
    "SteelAccent": build_color_material("M_RF_AccentSteel", (0.03, 0.36, 0.92), 0.15, 0.22, 1.2),
    "WoodAccent": build_color_material("M_RF_AccentWood", (0.95, 0.17, 0.025), 0.05, 0.30, 0.9),
    "GlassAccent": build_color_material("M_RF_AccentGlass", (0.02, 0.84, 0.63), 0.05, 0.18, 1.1),
    "Wwise": build_color_material("M_RF_WwiseCore", (0.004, 0.07, 0.12), 0.25, 0.24, 0.35),
}

report = {"status": "success", "surface_materials": surface_paths, "support_materials": support_paths}
REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("RESONANCE_FORGE_VISUAL_ASSETS_READY " + json.dumps(report, ensure_ascii=False))

"""Read local metadata/assets as data; do not import engine tools or generate a project."""
import hashlib
import json
from pathlib import Path
import re

engine = Path("F:/JericoEngine/JericoEngine")
output = Path(__file__).parent
def digest(path):
    data = path.read_bytes()
    return {"path": str(path.relative_to(engine)), "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}

metadata = {}
for path in (engine / "Gems").rglob("gem.json"):
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    metadata[data["gem_name"]] = (path, data)
seeds = ["Atom", "CameraFramework", "ImGui", "WhiteBox", "EditorPythonBindings", "PhysX5", "QtForPython", "PrimitiveAssets", "PrefabBuilder"]
closure = {}
missing = []
def visit(name):
    name = re.split(r"[<>=!~]", name)[0].strip()
    if name in closure:
        return
    if name not in metadata:
        missing.append(name)
        return
    path, data = metadata[name]
    deps = data.get("dependencies", [])
    closure[name] = {**digest(path), "dependencies": deps, "license": data.get("license")}
    for dependency in deps:
        visit(dependency if isinstance(dependency, str) else dependency["name"])
for name in seeds:
    visit(name)
templates = {}
for name in ("ScriptOnlyProject", "MinimalProject"):
    root = engine / "Templates" / name / "Template"
    files = []
    for path in root.rglob("*"):
        if path.is_file():
            with path.open("rb") as stream:
                pointer = stream.read(128).startswith(b"version https://git-lfs.github.com/spec/v1")
            files.append({**digest(path), "lfs_pointer": pointer})
    level = next((root / "Levels").rglob("*.prefab"))
    prefab = json.loads(level.read_text(encoding="utf-8-sig"))
    component_types, asset_strings = set(), set()
    def walk(value, key=""):
        if isinstance(value, dict):
            for name, item in value.items():
                walk(item, name)
        elif isinstance(value, list):
            for item in value:
                walk(item, key)
        elif isinstance(value, str):
            if key == "$type":
                component_types.add(value)
            if "asset" in key.lower() and value:
                asset_strings.add(value)
    walk(prefab)
    templates[name] = {"files": files, "level": digest(level), "component_types": sorted(component_types), "asset_strings": sorted(asset_strings)}
asset_paths = [
    "Gems/WhiteBox/Assets/materials/WhiteBoxDefault.material",
    "Gems/WhiteBox/Assets/textures/uvs/_dev_UV_debug_diff.tif",
    "Gems/WhiteBox/Assets/textures/uvs/_dev_UV_debug_spec.tif",
    "Gems/WhiteBox/Assets/textures/uvs/_dev_UV_debug_ddn.tif",
    "Gems/Atom/Feature/Common/Assets/Materials/Types/StandardPBR.materialtype",
    "Gems/Atom/Feature/Common/Assets/Materials/Presets/PBR/default_grid.material",
    "Gems/Atom/Feature/Common/Assets/Textures/Default/default_basecolor.tif",
    "Gems/Atom/Feature/Common/Assets/Textures/Default/default_normal.tif",
    "Gems/Atom/Feature/Common/Assets/Textures/Default/default_roughness.tif",
    "Gems/Atom/Feature/Common/Assets/Passes/MainPipeline.pass",
]
assets = []
for relative in asset_paths:
    path = engine / relative
    data = path.read_bytes()
    assets.append({**digest(path), "lfs_pointer": data.startswith(b"version https://git-lfs.github.com/spec/v1")})
result = {"schema": "JE_editor_minimal_inputs_v1", "scope": "Static data inspection only; no source/config generation or builds/tests", "proposed_seed_gems": seeds,
    "gem_dependency_metadata_closure": closure, "missing_gem_metadata": sorted(set(missing)),
    "network_or_multiplayer_gems_in_closure": [name for name in closure if re.search(r"amazon|aws|multiplayer|gamelift", name, re.I)],
    "templates": templates, "focal_assets": assets}
(output / "inputs.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"gems": sorted(closure), "missing": result["missing_gem_metadata"], "network": result["network_or_multiplayer_gems_in_closure"],
    "templates": {name: {"file_count": len(value["files"]), "lfs_files": [item["path"] for item in value["files"] if item["lfs_pointer"]], "level": value["level"], "types": value["component_types"], "asset_strings": value["asset_strings"]} for name, value in templates.items()}, "assets": assets}, indent=2))

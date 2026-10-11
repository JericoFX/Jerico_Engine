import ast, hashlib, json, pathlib, re
repo = pathlib.Path(r"F:\JericoEngine\JericoEngine")
manifest_path = pathlib.Path(r"F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-freeze-editor-fixture-parent.json")
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
inputs = manifest["inputs"]
checks = []
for item in inputs:
    path = repo / item["path"]
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest().upper()
    checks.append({"path": item["path"], "exists": path.is_file(), "bytes_expected": item["bytes"], "bytes_actual": len(data), "sha256_expected": item["sha256"], "sha256_actual": digest, "match": len(data) == item["bytes"] and digest == item["sha256"]})
ordered = sorted(checks, key=lambda x: str((repo / x["path"]).resolve()).casefold())
aggregate_text = "\n".join(f"{x['path']} {x['sha256_actual']} {x['bytes_actual']}" for x in ordered)
aggregate = hashlib.sha256(aggregate_text.encode("utf-8")).hexdigest().upper()
py_path = repo / "Projects/JEEditorFixture/Editor/Scripts/author_room.py"
py_tree = ast.parse(py_path.read_text(encoding="utf-8"), filename=str(py_path))
ast_summary = {"parse_ok": True, "function_names": [n.name for n in py_tree.body if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef))], "imports": sorted({a.name for n in ast.walk(py_tree) if isinstance(n, ast.Import) for a in n.names} | {n.module for n in ast.walk(py_tree) if isinstance(n, ast.ImportFrom) and n.module})}
json_checks = []
for item in inputs:
    p = repo / item["path"]
    if p.suffix.lower() in {".json", ".setreg", ".prefab", ".material"}:
        try:
            value = json.loads(p.read_text(encoding="utf-8"))
            json_checks.append({"path": item["path"], "valid": True, "root_type": type(value).__name__})
        except Exception as exc:
            json_checks.append({"path": item["path"], "valid": False, "error": f"{type(exc).__name__}: {exc}"})
project = json.loads((repo / "Projects/JEEditorFixture/project.json").read_text(encoding="utf-8"))
whitebox = (repo / "Gems/WhiteBox/Code/Source/Rendering/WhiteBoxMaterial.cpp").read_text(encoding="utf-8")
author = py_path.read_text(encoding="utf-8")
reflection = {"native_fields": all(f'->Field("{x}"' in whitebox for x in ("Tint", "UseTexture")), "native_edit_labels": all(f'QT_TRANSLATE_NOOP("WhiteBox", "{x}")' in whitebox for x in ("Tint", "Use Texture")), "author_labels": all(f'set_property(component, "{x}"' in author for x in ("Tint", "Use Texture")), "type_lookup_bus": '"FindComponentTypeIdsByEntityType"' in author, "property_path_disambiguation": 'len(matches) != 1' in author, "topology_guard": 'MeshVertexCount() != 8 or handle.MeshFaceCount() != 12' in author, "undo_batch": '"BeginUndoBatch"' in author and '"EndUndoBatch"' in author, "editor_api_only_imports": all(x in ast_summary["imports"] for x in ("azlmbr.bus", "azlmbr.editor", "azlmbr.entity", "azlmbr.math", "azlmbr.legacy.general"))}
result = {"manifest_status": manifest["status"], "manifest_file_count": manifest["fileCount"], "manifest_aggregate_expected": manifest["aggregateSha256"], "manifest_aggregate_actual": aggregate, "manifest_aggregate_match": aggregate == manifest["aggregateSha256"], "file_match_count": sum(x["match"] for x in checks), "file_mismatches": [x for x in checks if not x["match"]], "ast": ast_summary, "json_checks": json_checks, "project_metadata": {"project_id": project.get("project_id"), "script_only": project.get("script_only"), "gem_names": project.get("gem_names"), "gem_count": len(project.get("gem_names", []))}, "reflection_constraints": reflection, "runtime_status": "not run: author_room.py requires the live O3DE Editor azlmbr buses and an explicitly opened fixture level"}
print(json.dumps(result, indent=2))

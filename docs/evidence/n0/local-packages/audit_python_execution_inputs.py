import hashlib
import json
import pathlib
import struct

base = pathlib.Path(__file__).resolve().parent
acquisition = json.loads((base / "python-input-acquisition.json").read_text(encoding="utf-8-sig"))
external_records = [{key: record[key] for key in ("name", "commit", "sha256", "archive", "url")}
                    for record in acquisition if record["name"] not in ("cpython", "expat")]
(base / "python-externals-manifest.json").write_text(json.dumps(external_records, indent=2) + "\n", encoding="utf-8")
pe_audit = []
for record in acquisition:
    snapshot = json.loads((base / (record["name"] + "-python-input-snapshot.json")).read_text(encoding="utf-8"))
    for entry in snapshot["pe_files_not_executed"]:
        path = pathlib.Path(snapshot["source"]) / entry["path"]
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != entry["sha256"]:
            raise ValueError("Immutable binary source changed")
        offset = struct.unpack_from("<I", data, 0x3c)[0]
        if data[offset:offset + 4] != b"PE\0\0":
            raise ValueError("Invalid PE signature")
        machine = struct.unpack_from("<H", data, offset + 4)[0]
        architectures = {0x8664: "amd64", 0x14c: "win32", 0xaa64: "arm64", 0x1c4: "arm32"}
        architecture = architectures.get(machine)
        if architecture is None or entry["path"].split("/")[0] != architecture:
            raise ValueError("Unexpected binary architecture: " + entry["path"])
        pe_audit.append({"component": record["name"], "path": entry["path"], "sha256": entry["sha256"],
                         "architecture": architecture, "bytes": len(data), "code_executed": False})
tools = []
for name, path_string in (
    ("bootstrap", "C:/Users/Jerico/AppData/Local/Programs/Python/Python312/python.exe"),
    ("msbuild", "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe")):
    path = pathlib.Path(path_string)
    with path.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    tools.append({"name": name, "path": str(path), "sha256": digest, "bytes": path.stat().st_size})
result = {"pe_files": pe_audit, "tools": tools,
          "provenance_is_commit_and_hash_receipt_not_signature": True,
          "allowed_code_source": "Official python/cpython-bin-deps commits; execution remains assigned to Luna",
          "license_files": {record["name"]: json.loads((base / (record["name"] + "-python-input-snapshot.json")).read_text(encoding="utf-8"))["licenses"] for record in acquisition}}
(base / "python-execution-input-audit.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"pe_files": len(pe_audit), "all_architectures_match_payload_subdirectories": True, "tools": tools}))

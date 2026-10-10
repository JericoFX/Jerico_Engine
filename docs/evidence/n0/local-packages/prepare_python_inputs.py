import hashlib
import json
import pathlib
import shutil
import tarfile
import time

base = pathlib.Path(__file__).resolve().parent
dependencies = pathlib.Path("F:/JericoEngine/Dependencies")
records = json.loads((base / "python-input-acquisition.json").read_text(encoding="utf-8-sig"))
audit = json.loads((base / "python-input-tar-audit.json").read_text(encoding="utf-8")) if (base / "python-input-tar-audit.json").exists() else []
externals = json.loads((base / "python-externals-manifest.json").read_text(encoding="utf-8")) if (base / "python-externals-manifest.json").exists() else []
for record in records:
    started = time.perf_counter()
    archive = pathlib.Path(record["archive"])
    with archive.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != record["sha256"]:
            raise ValueError("Archive changed: " + archive.name)
    expected_root = record["repository"].split("/")[-1] + "-" + record["commit"]
    source = dependencies / (record["name"] + "-" + record["commit"])
    if source.exists():
        snapshot_path = base / (record["name"] + "-python-input-snapshot.json")
        snapshot = json.loads(snapshot_path.read_text(encoding="utf-8"))
        if snapshot["commit"] != record["commit"] or snapshot["archive_sha256"] != record["sha256"]:
            raise ValueError("Existing source snapshot identity differs")
        for file in snapshot["files"]:
            for existing_root in (source, pathlib.Path(snapshot["work"])):
                with (existing_root / file["path"]).open("rb") as stream:
                    if hashlib.file_digest(stream, "sha256").hexdigest() != file["sha256"]:
                        raise ValueError("Existing snapshot changed: " + file["path"])
        print(json.dumps({"name": record["name"], "resumed_existing_snapshot_hashes_match": True}), flush=True)
        continue
    with tarfile.open(archive, "r:gz") as package:
        members = package.getmembers()
        seen = set()
        for member in members:
            parts = pathlib.PurePosixPath(member.name).parts
            if not parts or parts[0] != expected_root or ".." in parts or "\\" in member.name or ":" in member.name or member.name.startswith("/"):
                raise ValueError("Unsafe/unexpected source archive path: " + member.name)
            allowed_readme_link = (record["name"] == "expat" and member.issym()
                                  and member.name == expected_root + "/README.md" and member.linkname == "expat/README.md")
            if not (member.isfile() or member.isdir() or allowed_readme_link):
                raise ValueError("Non-regular source archive entry: " + member.name + " -> " + member.linkname)
            if allowed_readme_link and not package.getmember(expected_root + "/expat/README.md").isfile():
                raise ValueError("Expat README link target is not a regular internal file")
            relative = "/".join(parts[1:])
            if not relative:
                if member.isdir():
                    continue
                raise ValueError("Unexpected archive root file")
            if relative.casefold() in seen:
                raise ValueError("Duplicate source path: " + relative)
            seen.add(relative.casefold())
        source.mkdir()
        files = []
        pe_files = []
        licenses = []
        for member in members:
            parts = pathlib.PurePosixPath(member.name).parts[1:]
            if not parts:
                continue
            path = source.joinpath(*parts)
            if member.isdir():
                path.mkdir(parents=True, exist_ok=True)
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            payload_member = package.getmember(expected_root + "/expat/README.md") if member.issym() else member
            with package.extractfile(payload_member) as payload, path.open("xb") as output:
                shutil.copyfileobj(payload, output)
            with path.open("rb") as stream:
                start = stream.read(128)
                stream.seek(0)
                digest = hashlib.file_digest(stream, "sha256").hexdigest()
            if start.startswith(b"version https://git-lfs.github.com/spec/v1"):
                raise ValueError("Unmaterialized LFS input: " + str(path))
            relative = "/".join(parts)
            entry = {"path": relative, "bytes": payload_member.size, "sha256": digest}
            if member.issym():
                entry["archive_symlink_materialized_as_regular_copy"] = member.linkname
            files.append(entry)
            if path.suffix.lower() in (".dll", ".exe", ".pyd"):
                if not start.startswith(b"MZ"):
                    raise ValueError("Expected PE header is missing: " + relative)
                pe_files.append(entry)
            if any(word in path.name.lower() for word in ("license", "copying", "copyright", "notice")):
                licenses.append(relative)
    if record["name"] in ("cpython", "expat"):
        work = base / "source-work" / source.name
    else:
        work = base / "python-externals" / record["name"]
        externals.append({key: record[key] for key in ("name", "commit", "sha256", "archive")})
    if work.exists():
        raise ValueError("Work destination already exists: " + str(work))
    shutil.copytree(source, work)
    snapshot = {"name": record["name"], "commit": record["commit"], "archive_sha256": record["sha256"],
                "source": str(source), "work": str(work), "files": files,
                "licenses": licenses, "pe_files_not_executed": pe_files,
                "only_regular_files_and_directories_after_explicit_readme_link_materialization": True,
                "elapsed_seconds": round(time.perf_counter() - started, 3)}
    (base / (record["name"] + "-python-input-snapshot.json")).write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
    audit.append({key: value for key, value in snapshot.items() if key != "files"})
    (base / "python-input-tar-audit.json").write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    (base / "python-externals-manifest.json").write_text(json.dumps(externals, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"name": record["name"], "files": len(files), "bytes": sum(file["bytes"] for file in files),
                      "licenses": licenses, "pe_files": len(pe_files), "work": str(work)}), flush=True)

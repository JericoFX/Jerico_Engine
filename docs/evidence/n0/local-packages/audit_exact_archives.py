import hashlib
import json
import pathlib
import re
import tarfile
import time

root = pathlib.Path(__file__).resolve().parent
records = json.loads((root / "exact-actions-artifacts.json").read_text(encoding="utf-8-sig"))
results = []
for record in records:
    started = time.perf_counter()
    archive = pathlib.Path(record["package_archive"])
    with archive.open("rb") as stream:
        actual = hashlib.file_digest(stream, "sha256").hexdigest()
    if actual != record["package_sha256"]:
        raise ValueError("Archive changed: " + archive.name)
    destination = root / "exact-package-inspection-rev2" / record["name"]
    destination.mkdir(parents=True, exist_ok=False)
    entries = []
    hashes = {}
    selected = []
    links = []
    total = 0
    manifest = None
    info = None
    with tarfile.open(archive, "r|xz") as package:
        for member in package:
            parts = pathlib.PurePosixPath(member.name).parts
            if member.name.startswith("/") or "\\" in member.name or ":" in member.name or ".." in parts:
                raise ValueError("Unsafe archive path: " + member.name)
            if not (member.isfile() or member.isdir() or member.islnk()):
                raise ValueError("Non-regular archive entry: " + member.name)
            name = "/".join(part for part in parts if part != ".")
            if not name:
                if member.isdir():
                    continue
                raise ValueError("Empty file path")
            if name in hashes or any(entry["name"] == name for entry in entries):
                raise ValueError("Duplicate archive path: " + name)
            entry = {"name": name, "kind": "dir" if member.isdir() else "hardlink" if member.islnk() else "file", "bytes": member.size}
            entries.append(entry)
            if member.isdir():
                continue
            if member.islnk():
                link_parts = pathlib.PurePosixPath(member.linkname).parts
                if member.linkname.startswith("/") or "\\" in member.linkname or ":" in member.linkname or ".." in link_parts:
                    raise ValueError("Unsafe hardlink target: " + member.linkname)
                entry["target"] = "/".join(part for part in link_parts if part != ".")
                links.append(entry)
                continue
            keep = (name.endswith((".cmake", ".json")) or pathlib.PurePosixPath(name).name.lower() in
                    ("setup.py", "pyproject.toml", "setup.cfg", "sha256sums", "license", "license.txt", "qt-notice.txt", "notice.txt"))
            keep = keep and member.size <= 2 * 1024 * 1024
            target = None
            if keep:
                path = destination.joinpath(*parts)
                path.parent.mkdir(parents=True, exist_ok=True)
                target = path.open("xb")
                selected.append(name)
            digest = hashlib.sha256()
            payload = package.extractfile(member)
            try:
                while chunk := payload.read(1024 * 1024):
                    digest.update(chunk)
                    if target:
                        target.write(chunk)
            finally:
                payload.close()
                if target:
                    target.close()
            hashes[name] = digest.hexdigest()
            entry["sha256"] = hashes[name]
            total += member.size
            if name == "SHA256SUMS":
                manifest = (destination / name).read_text(encoding="utf-8-sig")
            if name == "PackageInfo.json":
                info = json.loads((destination / name).read_text(encoding="utf-8-sig"))
    for link in links:
        if link["target"] not in hashes:
            raise ValueError("Hardlink target must be an internal regular file: " + link["target"])
        hashes[link["name"]] = hashes[link["target"]]
        link["sha256"] = hashes[link["target"]]
    if manifest is None or info is None:
        raise ValueError("Expected package metadata is missing")
    checked = set()
    for line in manifest.splitlines():
        match = re.fullmatch(r"([a-fA-F0-9]{64})\s+\*?(.+)", line)
        if not match:
            raise ValueError("Malformed content manifest")
        name = match.group(2).removeprefix("./")
        if name in checked or hashes.get(name) != match.group(1).lower():
            raise ValueError("Payload digest mismatch: " + name)
        checked.add(name)
    if checked != set(hashes) - {"SHA256SUMS"}:
        raise ValueError("Content manifest does not cover the entire payload")
    (destination / "parent-tar-inventory.json").write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")
    result = {"name": record["name"], "archive_sha256": actual, "files": len(hashes), "entries": len(entries),
              "payload_bytes": total, "manifest_entries_verified": len(checked), "regular_payload_and_internal_file_hardlinks_only": True,
              "hardlinks": links,
              "package_info": info, "selected_text_files": len(selected), "inspection_path": str(destination),
              "elapsed_seconds": round(time.perf_counter() - started, 3), "code_executed_from_package": False}
    results.append(result)
    print(json.dumps({key: value for key, value in result.items() if key != "package_info"}), flush=True)
(root / "exact-package-tar-audit.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")

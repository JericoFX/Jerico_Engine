"""Static tar inspection/extraction; never imports or executes acquired source."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile

audit = Path(__file__).parent
receipt = json.loads((audit / "source-acquisition.json").read_text(encoding="utf-8-sig"))
archive = Path(receipt["archive"])
assert hashlib.sha256(archive.read_bytes()).hexdigest() == receipt["sha256"]
commit = receipt["upstream_commit"]
tree = json.loads((audit / "upstream-tree.json").read_text(encoding="utf-8"))
assert tree["sha"] == commit and not tree["truncated"]
blobs = {item["path"]: item for item in tree["tree"] if item["type"] == "blob"}
root = "rapidjson-" + commit
destination = Path("F:/JericoEngine/Dependencies") / root
assert not destination.exists(), "Refusing existing snapshot"
entries = []
files = {}
seen = set()
with tarfile.open(archive, "r:gz") as package:
    for member in package.getmembers():
        name = member.name.rstrip("/")
        parts = PurePosixPath(name).parts
        assert parts and parts[0] == root
        assert not name.startswith("/") and "\\" not in name
        assert all(part not in (".", "..") and ":" not in part and not part.endswith((".", " ")) for part in parts)
        key = name.casefold()
        assert key not in seen, "Case/path collision"
        seen.add(key)
        assert member.isdir() or member.isfile(), "Link/special member rejected"
        relative = "/".join(parts[1:])
        entry = {"path": relative, "type": "directory" if member.isdir() else "file", "bytes": member.size}
        if member.isfile():
            data = package.extractfile(member).read()
            assert len(data) == member.size
            blob_sha = hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()
            assert relative in blobs and blobs[relative]["mode"] in ("100644", "100755")
            assert blob_sha == blobs[relative]["sha"], "Pinned Git tree blob mismatch"
            files[relative] = data
            entry.update(sha256=hashlib.sha256(data).hexdigest(), git_blob_sha1=blob_sha)
        entries.append(entry)
assert set(files) == set(blobs), "Archive must exactly match all pinned source blobs"
headers = {path: data for path, data in files.items() if path.startswith("include/")}
license_text = files["license.txt"].decode("utf-8")
assert "Terms of the MIT License:" in license_text and "Terms of the JSON License:" in license_text
assert headers and "include/rapidjson/rapidjson.h" in headers
for path, data in files.items():
    output = destination.joinpath(*PurePosixPath(path).parts)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(data)
for path, data in files.items():
    assert hashlib.sha256(destination.joinpath(*PurePosixPath(path).parts).read_bytes()).digest() == hashlib.sha256(data).digest()
summary = {
    "schema": "JE_RapidJSON_source_tar_audit_v1", "archive": receipt,
    "snapshot": str(destination), "member_count": len(entries), "file_count": len(files),
    "payload_bytes": sum(len(data) for data in files.values()), "links": 0,
    "all_regular_files_match_pinned_git_tree": True, "extraction_hashes_match": True,
    "header_count": len(headers), "header_bytes": sum(len(data) for data in headers.values()),
    "license_sha256": hashlib.sha256(files["license.txt"]).hexdigest(),
    "payload_license_notes": "Header payload MIT plus bundled msinttypes BSD-3-Clause. Full license text retained. JSON-licensed bin/jsonchecker excluded from proposed header package; source snapshot still contains it.",
    "entries": entries,
}
(audit / "source-tar-audit.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print(json.dumps({key: value for key, value in summary.items() if key != "entries"}, indent=2))

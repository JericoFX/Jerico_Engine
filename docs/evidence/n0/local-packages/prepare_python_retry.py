import hashlib
import json
import pathlib
import shutil
import time

base = pathlib.Path(__file__).resolve().parent
snapshot = json.loads((base / "cpython-python-input-snapshot.json").read_text(encoding="utf-8-sig"))
source = pathlib.Path(snapshot["source"])
work = base / "source-work" / (source.name + "-rev2")
if work.exists():
    raise ValueError("Fresh work destination already exists")
started = time.perf_counter()
expected = {entry["path"]: entry["sha256"] for entry in snapshot["files"]}
for root in (source,):
    actual = {path.relative_to(root).as_posix() for path in root.rglob("*") if path.is_file()}
    if actual != set(expected):
        raise ValueError("Immutable CPython file set differs")
    for path in root.rglob("*"):
        if path.is_symlink():
            raise ValueError("Unexpected link: " + str(path))
    for relative, digest in expected.items():
        with (root / relative).open("rb") as stream:
            if hashlib.file_digest(stream, "sha256").hexdigest() != digest:
                raise ValueError("Immutable CPython hash differs: " + relative)
shutil.copytree(source, work)
for relative, digest in expected.items():
    with (work / relative).open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != digest:
            raise ValueError("Fresh work hash differs: " + relative)
snapshot["previous_failed_work"] = snapshot["work"]
snapshot["work"] = str(work)
snapshot["elapsed_seconds"] = round(time.perf_counter() - started, 3)
(base / "cpython-python-input-snapshot-rev2.json").write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"files": len(expected), "work": str(work), "sha256_all_match": True}))

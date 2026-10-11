import hashlib
import json
import pathlib
import shutil
import tarfile


repo = pathlib.Path(r"F:\JericoEngine\JericoEngine")
base = repo / "build/n0-validation/local-packages"
producer = base / "lz4-v143-rev1"
packages = producer / "packages"
name = "lz4-1.9.4-je-rev1-windows"
archive = packages / f"{name}.tar.xz"
sidecar = pathlib.Path(str(archive) + ".SHA256SUMS")
contents = packages / f"{name}.contents"
file_list = packages / f"{name}.files"
extract_root = base / "lz4-extracted-rev1"
ev = base / "lz4-rev1-validation"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


if extract_root.exists():
    raise SystemExit(f"Refusing to overwrite extraction: {extract_root}")

sidecar_line = sidecar.read_text(encoding="ascii").strip()
sidecar_hash, sidecar_name = sidecar_line.split(" *", 1)
archive_hash = sha256(archive)
archive_bytes = archive.stat().st_size
if sidecar_name != archive.name or sidecar_hash.upper() != archive_hash:
    raise SystemExit("Archive sidecar name/hash mismatch")

manifest_path = contents / "SHA256SUMS"
manifest = {}
for line in manifest_path.read_text(encoding="ascii").splitlines():
    expected_hash, rel = line.split(" *", 1)
    if rel in manifest:
        raise SystemExit(f"Duplicate SHA256SUMS path: {rel}")
    manifest[rel] = expected_hash.upper()

content_files = sorted(p for p in contents.rglob("*") if p.is_file())
content_rel = {p.relative_to(contents).as_posix(): p for p in content_files}
content_hash_mismatches = []
for rel, expected_hash in manifest.items():
    path = content_rel.get(rel)
    if path is None or sha256(path) != expected_hash:
        content_hash_mismatches.append(rel)
expected_members = set(manifest) | {"SHA256SUMS"}
if set(content_rel) != expected_members:
    content_hash_mismatches.extend(sorted(set(content_rel) ^ expected_members))

listed = [line.removeprefix("--add-file=") for line in file_list.read_text(encoding="utf-8").splitlines()]
members = []
unsafe_members = []
member_hash_mismatches = []
member_count = 0
member_bytes = 0
with tarfile.open(archive, mode="r:xz") as tar:
    for member in tar.getmembers():
        member_count += 1
        posix_path = pathlib.PurePosixPath(member.name)
        if (not member.isfile() or member.issym() or member.islnk() or member.issparse()
                or posix_path.is_absolute() or ".." in posix_path.parts
                or ":" in member.name or "\\" in member.name or member.name != posix_path.as_posix()):
            unsafe_members.append({"name": member.name, "type": member.type.decode("ascii", "replace")})
            continue
        rel = posix_path.as_posix()
        members.append(rel)
        member_bytes += member.size
        extracted = tar.extractfile(member)
        if extracted is None:
            member_hash_mismatches.append(rel)
            continue
        digest = hashlib.sha256()
        while True:
            chunk = extracted.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
        expected = manifest.get(rel)
        if rel == "SHA256SUMS":
            expected = sha256(manifest_path)
        if expected is None or digest.hexdigest().upper() != expected:
            member_hash_mismatches.append(rel)

if unsafe_members:
    raise SystemExit(f"Unsafe archive entries: {unsafe_members[:5]}")
if sorted(members) != sorted(listed) or set(members) != expected_members:
    raise SystemExit("Tar members do not exactly match the package file list and checksum manifest")
if content_hash_mismatches or member_hash_mismatches:
    raise SystemExit(f"Payload hash mismatches: contents={content_hash_mismatches[:5]}, tar={member_hash_mismatches[:5]}")

info = json.loads((contents / "PackageInfo.json").read_text(encoding="utf-8"))
expected_info = {
    "name": name,
    "version": "1.9.4",
    "license": "BSD-2-Clause",
    "source": {
        "url": "https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4",
        "commit": "5ff839680134437dbf4678f3d0c7b371d84f4964",
        "sha256": "0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b",
    },
}
if info != {"schemaVersion": 1, **expected_info}:
    raise SystemExit(f"Unexpected package metadata: {info}")

extract_root.mkdir(parents=True, exist_ok=False)
with tarfile.open(archive, mode="r:xz") as tar:
    for member in tar.getmembers():
        target = extract_root.joinpath(*pathlib.PurePosixPath(member.name).parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        with tar.extractfile(member) as src, target.open("xb") as dst:
            shutil.copyfileobj(src, dst)

extracted_files = sorted(p for p in extract_root.rglob("*") if p.is_file())
extracted_mismatches = []
for rel, expected_hash in {**manifest, "SHA256SUMS": sha256(manifest_path)}.items():
    path = extract_root.joinpath(*pathlib.PurePosixPath(rel).parts)
    if not path.is_file() or sha256(path) != expected_hash:
        extracted_mismatches.append(rel)
if {p.relative_to(extract_root).as_posix() for p in extracted_files} != expected_members or extracted_mismatches:
    raise SystemExit(f"Extraction mismatch: {extracted_mismatches[:5]}")

lib = extract_root / "lz4/lib/lz4.lib"
recipe = json.loads((extract_root / "lz4/JERecipe.json").read_text(encoding="utf-8"))
summary = {
    "archive": str(archive),
    "archive_bytes": archive_bytes,
    "archive_sha256": archive_hash,
    "sidecar_matches": True,
    "package_file_manifest_count": len(manifest),
    "package_contents_file_count": len(content_files),
    "content_hash_mismatches": content_hash_mismatches,
    "tar_member_count": member_count,
    "tar_member_bytes": member_bytes,
    "tar_all_regular_and_safe": not unsafe_members,
    "tar_members_match_file_list_and_manifest": True,
    "tar_payload_hash_mismatches": member_hash_mismatches,
    "package_metadata": info,
    "extraction_root": str(extract_root),
    "extracted_file_count": len(extracted_files),
    "extraction_hash_mismatches": extracted_mismatches,
    "recipe": recipe,
    "library": {"path": str(lib), "bytes": lib.stat().st_size, "sha256": sha256(lib)},
    "headers": {
        name: {"bytes": (extract_root / "lz4/include" / name).stat().st_size,
               "sha256": sha256(extract_root / "lz4/include" / name)}
        for name in ("lz4.h", "lz4hc.h", "lz4frame.h")
    },
    "pass": True,
}
(ev / "package-audit.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print(json.dumps(summary, indent=2))

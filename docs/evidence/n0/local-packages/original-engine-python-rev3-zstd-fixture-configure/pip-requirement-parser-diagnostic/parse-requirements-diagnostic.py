import hashlib, json, pathlib, re, sys
from pip._internal.network.session import PipSession
from pip._internal.req.req_file import parse_requirements
from packaging.requirements import Requirement
root = pathlib.Path(r"F:\JericoEngine\JericoEngine")
source = root / "python/requirements.txt"
build_source = root / "python/requirements-build.txt"
expected_source_sha = "22f69d842f651cce2738ff5e31a302bd5f1923c5fdc2b9167a897e13bc20c5a2"
raw = source.read_bytes()
source_sha_before = hashlib.sha256(raw).hexdigest()
if source_sha_before != expected_source_sha:
    raise RuntimeError(f"requirements.txt source hash changed: {source_sha_before}")
active = [line.decode("utf-8", "replace") for line in raw.splitlines() if line.strip() and not line.lstrip().startswith(b"#")]
if any(re.match(r"\s*(?:-r\b|--requirement\b|-f\b|--find-links\b|https?://)", line) for line in active):
    raise RuntimeError("Unexpected include or remote source in requirements; parser-only boundary refused")
def parse(path):
    rows = []
    session = PipSession()
    for item in parse_requirements(str(path), session=session):
        requirement_text = getattr(item, "requirement", None)
        try:
            name = Requirement(requirement_text).name if requirement_text else None
            specifier = str(Requirement(requirement_text).specifier) if requirement_text else None
        except Exception as exc:
            name, specifier = None, None
            rows.append({"parse_error": f"{type(exc).__name__}: {exc}", "requirement": requirement_text})
        rows.append({"name": name, "specifier": specifier, "requirement": requirement_text, "comes_from": str(getattr(item, "comes_from", "")), "line_source": str(getattr(item, "line_source", "")), "is_editable": getattr(item, "is_editable", None), "options": getattr(item, "options", None)})
    return rows
lexical = []
for number, line in enumerate(raw.decode("utf-8").splitlines(), 1):
    m = re.match(r"\s*([A-Za-z0-9_.-]+)==([^\s\\]+)", line)
    if m:
        lexical.append({"line": number, "name": m.group(1), "version": m.group(2), "trailing_continuation": line.rstrip().endswith("\\")})
original_rows = parse(source)
build_rows = parse(build_source)
copy_path = pathlib.Path(sys.argv[1])
copy_path.parent.mkdir(parents=True, exist_ok=True)
needle = re.compile(rb"(--hash=sha256:e920276dd6813095e9377c0bc5566d94c932c33b27a3e3945d8389c374dd4746)[ \t]*\\(?=\r?$)", re.MULTILINE)
modified, replacements = needle.subn(rb"\1", raw)
if replacements != 1:
    raise RuntimeError(f"expected one exact continuation edit in evidence copy, got {replacements}")
copy_path.write_bytes(modified)
copy_sha = hashlib.sha256(modified).hexdigest()
copy_rows = parse(copy_path)
def names(rows):
    return [r["name"] for r in rows if r.get("name")]
result = {"pip_version": __import__("pip").__version__, "parser": "pip._internal.req.req_file.parse_requirements with local PipSession; no pip install or network calls", "input_hashes": {"requirements_txt": source_sha_before, "requirements_build_txt": hashlib.sha256(build_source.read_bytes()).hexdigest(), "requirements_copy": copy_sha}, "physical_lines_1_to_5": raw.decode("utf-8").splitlines()[:5], "lexical_pins": lexical, "original_requirements_txt": {"rows": original_rows, "names": names(original_rows), "atomicwrites_present": "atomicwrites" in names(original_rows)}, "requirements_build_txt": {"rows": build_rows, "names": names(build_rows)}, "evidence_copy": {"path": str(copy_path), "only_change": "removed trailing backslash from the exact pluggy hash line; source is untouched", "parsed_rows": copy_rows, "names": names(copy_rows), "atomicwrites_present": "atomicwrites" in names(copy_rows)}, "source_hash_after": hashlib.sha256(source.read_bytes()).hexdigest()}
result["source_unchanged"] = result["source_hash_after"] == expected_source_sha
print(json.dumps(result, indent=2, default=str))
if not result["source_unchanged"]:
    raise RuntimeError("source requirements changed during read-only parser test")
if not result["original_requirements_txt"]["atomicwrites_present"]:
    print("READ_ONLY_REGRESSION_FAILED: atomicwrites lexical pin was not emitted by pip parser", file=sys.stderr)
    raise SystemExit(1)

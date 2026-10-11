import hashlib, json, os, pathlib, zipfile

out = pathlib.Path(__file__).parent
audit = json.loads((out / 'zip-audit.json').read_text())
archive = pathlib.Path(audit['archive'])
root = pathlib.Path(r'F:\JericoEngine\Dependencies\tools\strawberry-perl-5.42.3.1-64bit-portable')
if root.exists() or not audit['safe_for_regular_file_extraction']:
    raise SystemExit('Existing destination or unsafe ZIP')
if hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest() != audit['sha256']:
    raise SystemExit('Archive changed')
for parent in root.parents:
    if parent.exists() and parent.stat().st_file_attributes & 0x400:
        raise SystemExit('Reparse ancestor rejected')
root.mkdir(parents=True)
manifest = []
with zipfile.ZipFile(archive) as bundle:
    for entry in bundle.infolist():
        target = root.joinpath(*pathlib.PurePosixPath(entry.filename).parts)
        if os.path.commonpath((str(root), str(target.resolve()))) != str(root):
            raise SystemExit('Escaping destination')
        if entry.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            data = bundle.read(entry)
            with target.open('xb') as stream:
                stream.write(data)
            actual = hashlib.file_digest(target.open('rb'), 'sha256').hexdigest()
            if actual != hashlib.sha256(data).hexdigest():
                raise SystemExit('Extracted byte mismatch')
            manifest.append({'path': entry.filename, 'bytes': len(data), 'sha256': actual})
reparse = [str(path.relative_to(root)) for path in root.rglob('*') if path.stat().st_file_attributes & 0x400]
if reparse or len(manifest) != audit['files']:
    raise SystemExit('Extraction tree mismatch')
for name, data in [('tool-manifest.json', manifest), ('tool-extraction.json', {'root': str(root), 'files': len(manifest), 'payload_bytes': sum(row['bytes'] for row in manifest), 'all_hashes_match': True, 'reparse_paths': reparse, 'tool_executed': False, 'policy': 'Immutable internal build tool input; no global installation/PATH mutation, shell/relocation/CPAN scripts executed, or runtime redistribution'})]:
    with (out / name).open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)
print('Extracted and verified', len(manifest), 'files; no tool execution')

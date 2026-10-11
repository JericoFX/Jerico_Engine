import hashlib, json, pathlib, re, stat, unicodedata, zipfile

out = pathlib.Path(__file__).parent
archive = pathlib.Path(r'F:\JericoEngine\Dependencies\archives\strawberry-perl-5.42.3.1-64bit-portable.zip')
expected = '6a081a811781c30aca51dbc036afd93092af91e3297901f02c17043795a10690'
if hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest() != expected:
    raise SystemExit('Archive pin mismatch')
reserved = re.compile(r'^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)', re.I)
members, issues, names, legal = [], [], {}, []
with zipfile.ZipFile(archive) as bundle:
    for entry in bundle.infolist():
        path = pathlib.PurePosixPath(entry.filename)
        parts = entry.filename.rstrip('/').split('/')
        errors = []
        if path.is_absolute() or '\\' in entry.filename or any(part in ('', '.', '..') or ':' in part or part.rstrip(' .') != part or reserved.match(part) for part in parts):
            errors.append('Unsafe Windows path')
        normalized = unicodedata.normalize('NFC', '/'.join(path.parts)).casefold()
        if normalized in names:
            errors.append('Windows path collision')
        names[normalized] = entry.filename
        mode = entry.external_attr >> 16
        kind = stat.S_IFMT(mode)
        if kind not in (0, stat.S_IFREG, stat.S_IFDIR) or entry.external_attr & 0x400:
            errors.append('Link/special/reparse member')
        if entry.flag_bits & 1:
            errors.append('Encrypted member')
        row = {'path': entry.filename, 'type': 'directory' if entry.is_dir() else 'file', 'bytes': entry.file_size, 'compressed_bytes': entry.compress_size, 'mode': oct(mode), 'crc32': f'{entry.CRC:08x}', 'errors': errors}
        if not entry.is_dir():
            data = bundle.read(entry)  # Validates the ZIP CRC and full decompression.
            row['sha256'] = hashlib.sha256(data).hexdigest()
            if not errors and re.search(r'licen[cs]e|copying|copyright|notice|artistic', path.name, re.I):
                target = out / 'license-text' / pathlib.Path(*path.parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                with target.open('xb') as stream:
                    stream.write(data)
                legal.append({'path': entry.filename, 'sha256': row['sha256'], 'bytes': len(data)})
        if errors:
            issues.append(row)
        members.append(row)
by_name = {row['path'].rstrip('/'): row for row in members}
for row in members:
    for parent in pathlib.PurePosixPath(row['path']).parents:
        if str(parent) in by_name and by_name[str(parent)]['type'] != 'directory':
            row['errors'].append('File ancestor collision')
            issues.append(row)
summary = {'archive': str(archive), 'sha256': expected, 'members': len(members), 'files': sum(row['type'] == 'file' for row in members), 'directories': sum(row['type'] == 'directory' for row in members), 'payload_bytes': sum(row['bytes'] for row in members), 'issues': issues, 'safe_for_regular_file_extraction': not issues, 'licenses': legal, 'tool_executed': False}
for name, data in [('zip-members.json', members), ('zip-audit.json', summary)]:
    with (out / name).open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)
print(json.dumps({key: value for key, value in summary.items() if key != 'licenses'}, indent=2))
print('License files:', len(legal))
if issues:
    raise SystemExit('ZIP rejected; extraction forbidden')

import hashlib, json, pathlib, re

out = pathlib.Path(__file__).parent
root = pathlib.Path(r'F:\JericoEngine\Dependencies\openssl-1.1.1o')
previous = out.parent / 'openssl-current-audit'
records = json.loads((out / 'source-manifest.json').read_text())

def write_new(name, data):
    target = out / name
    with target.open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)

def redact(value):
    if isinstance(value, dict):
        return {key: redact(item) for key, item in value.items()}
    if isinstance(value, list):
        return [redact(item) for item in value]
    if isinstance(value, str):
        return re.sub(r'(https?://[^\s?\"<>]+)\?[^\s\"<>]*', r'\1?<redacted>', value)
    return value

for name in ('official-source-head-hop1.json', 'official-source-head-hop2.json'):
    source = previous / name
    data = redact(json.loads(source.read_text(encoding='utf-8-sig')))
    data['sanitization'] = 'New public copy; original private frozen receipt unchanged; URL queries redacted recursively'
    data['privateOriginalSha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
    write_new('sanitized-' + name, data)

notices = []
closure = []
special = []
notice_pattern = re.compile(r'copyright|licen[cs]e|redistribut|public domain|cryptogams|artistic|permission is hereby|GNU General|GNU Lesser|Mozilla Public', re.I)
special_pattern = re.compile(r'public domain|cryptogams|artistic|permission is hereby|GNU General|GNU Lesser|Mozilla Public|dual.licensed|redistribution and use', re.I)
network_pattern = re.compile(r'\bcurl\b|\bwget\b|git\s+(clone|submodule)|FetchContent|ExternalProject|LWP::|HTTP::Tiny|urlopen|requests\.get|CPAN::Shell|\bcpanm\b', re.I)
for record in records:
    path = root / record['path']
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != record['sha256']:
        raise SystemExit('Immutable source changed: ' + record['path'])
    if b'\0' in raw:
        continue
    text = raw.decode('utf-8', errors='replace')
    lines = text.splitlines()
    found = [{'line': i + 1, 'text': line} for i, line in enumerate(lines) if notice_pattern.search(line)]
    if found:
        notices.append({'path': record['path'], 'sha256': record['sha256'], 'notice_lines': found})
    if special_pattern.search(text):
        target = out / 'full-additional-notices' / record['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(raw)
        special.append({'path': record['path'], 'sha256': record['sha256'], 'full_byte_copy': str(target.relative_to(out)).replace('\\', '/')})
    rel = pathlib.PurePosixPath(record['path'])
    is_build_input = (rel.name in ('Configure', 'config', 'build.info') or
                      rel.parts[0] == 'Configurations' or
                      rel.parts[0] == 'util' and (rel.suffix in ('.pl', '.pm', '.tmpl') or rel.name == 'dofile.pl') or
                      rel.parts[0] == 'external' and rel.suffix == '.pm')
    if is_build_input:
        matches = [{'line': i + 1, 'text': line} for i, line in enumerate(lines) if network_pattern.search(line)]
        closure.append({'path': record['path'], 'sha256': record['sha256'], 'potential_network_marker_lines': matches})

write_new('all-source-notice-lines.json', notices)
write_new('full-additional-notices-manifest.json', special)
write_new('build-input-static-scan.json', closure)
write_new('static-scan-summary.json', {
    'immutable_source_files_verified': len(records),
    'source_files_with_notices': len(notices),
    'additional_notice_files_preserved_full_bytes': len(special),
    'build_input_files_scanned': len(closure),
    'build_inputs_with_potential_network_markers': [item for item in closure if item['potential_network_marker_lines']],
    'scope': 'Static source inspection only; markers are evidence for review, not executed behavior or final legal/build acceptance',
    'all_original_notice_bytes_remain_in_immutable_snapshot': True
})
print(json.dumps({'verified_source_files': len(records), 'notice_files': len(notices), 'additional_notice_files': len(special), 'build_inputs': len(closure), 'potential_network_marker_files': sum(bool(item['potential_network_marker_lines']) for item in closure)}, indent=2))

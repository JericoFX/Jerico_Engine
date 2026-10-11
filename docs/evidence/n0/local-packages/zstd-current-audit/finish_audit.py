import fnmatch, hashlib, json, pathlib, re

out = pathlib.Path(r'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\zstd-current-audit')
source = pathlib.Path(r'F:\JericoEngine\Dependencies\zstd-90ae50224d15e8dbcb9fa26b9be096366733db8e')
tree = json.loads((out / 'upstream-tree.txt').read_text(encoding='utf-8'))
if tree['sha'] != '90ae50224d15e8dbcb9fa26b9be096366733db8e' or tree['truncated']:
    raise SystemExit('Wrong/incomplete primary tree')
blobs = {x['path']: x for x in tree['tree'] if x['type'] == 'blob'}
manifest = json.loads((out / 'source-manifest.json').read_text(encoding='utf-8'))
if set(blobs) != {x['path'] for x in manifest}:
    raise SystemExit('Primary tree/archive file paths mismatch')
comparisons = []
eol_paths = []
for item in manifest:
    data = (source / item['path']).read_bytes()
    sha256 = hashlib.sha256(data).hexdigest()
    sha1 = hashlib.sha1(b'blob ' + str(len(data)).encode('ascii') + b'\0' + data).hexdigest()
    if sha256 != item['sha256']:
        raise SystemExit('Source SHA256 changed: ' + item['path'])
    canonical = data.replace(b'\r\n', b'\n')
    canonical_sha1 = hashlib.sha1(b'blob ' + str(len(canonical)).encode('ascii') + b'\0' + canonical).hexdigest()
    expected = blobs[item['path']]['sha']
    eol_only = sha1 != expected
    if eol_only:
        patterns = ('*.sln', '*.vcxproj*', '*.vcproj*', '*.rc', '*.bat', '*.cmd')
        if canonical_sha1 != expected or not any(fnmatch.fnmatch(pathlib.PurePosixPath(item['path']).name, p) for p in patterns):
            raise SystemExit('Unexplained primary blob/source mismatch: ' + item['path'])
        eol_paths.append(item['path'])
    comparisons.append({'path': item['path'], 'bytes': len(data), 'sha256': sha256,
                        'archive_blob_sha1': sha1, 'primary_git_blob_sha1': expected,
                        'eol_attributes_only': eol_only, 'lf_normalized_blob_sha1': canonical_sha1,
                        'primary_git_mode': blobs[item['path']]['mode']})
cmake = []
for path in source.rglob('*'):
    if path.is_file() and (path.name == 'CMakeLists.txt' or '.cmake' in path.name):
        text = path.read_text(encoding='utf-8')
        found = [{'line': n, 'text': line} for n, line in enumerate(text.splitlines(), 1)
                 if re.search(r'FetchContent|ExternalProject|file\s*\(\s*DOWNLOAD|execute_process|exec_program|git\s|wget|curl|find_package|add_subdirectory|configure_file|clean-all', line, re.I)]
        cmake.append({'path': str(path.relative_to(source)).replace('\\', '/'),
                      'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'closure_matches': found})
legal = []
for path in (source / 'lib').rglob('*'):
    if path.is_file() and path.suffix in ('.c', '.h'):
        text = path.read_text(encoding='utf-8')
        header = re.match(r'\s*/\*.*?\*/', text, re.S)
        if header:
            legal.append({'path': str(path.relative_to(source)).replace('\\', '/'),
                          'notice': header.group(0).strip(),
                          'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
result = {'commit': tree['sha'], 'primary_blobs_compared': len(comparisons),
          'all_source_sha256_match': True, 'primary_blob_exact_matches': len(comparisons) - len(eol_paths),
          'primary_blob_attribute_eol_matches': len(eol_paths), 'attribute_eol_paths': eol_paths,
          'eol_explanation': 'Only declared CRLF files in upstream .gitattributes differ from Git blobs. LF normalization reproduces the primary blob SHA1; snapshot retains exact archive bytes.',
          'cmake_files': len(cmake),
          'submodules': [x for x in tree['tree'] if x['type'] == 'commit'],
          'gitmodules_paths': [x for x in blobs if pathlib.PurePosixPath(x).name == '.gitmodules'],
          'validation_scope': 'Read-only source identity, extraction and static closure audit; no configure/build/test',
          'verification_order': 'SHA256 tar manifest generated before extraction; primary Git blob SHA1 comparison performed after extraction'}
for name, value in [('source-git-blob-manifest.json', comparisons),
                    ('cmake-closure-audit.json', cmake), ('library-legal-notices.json', legal),
                    ('source-identity-audit.json', result)]:
    path = out / name
    with path.open('x', encoding='utf-8') as stream:
        json.dump(value, stream, indent=2)
print(json.dumps(result, indent=2))

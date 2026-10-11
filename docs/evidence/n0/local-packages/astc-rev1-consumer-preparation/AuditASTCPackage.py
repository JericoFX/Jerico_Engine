# Static archive/data inspection only. Never imports or executes ASTC payload.
import hashlib, json, pathlib, re, shutil, struct, tarfile
root = pathlib.Path('F:/JericoEngine/JericoEngine')
base = root / 'build/n0-validation/local-packages'
ev = base / 'astc-rev1-consumer-preparation'
name = 'astc-encoder-3.2-je-rev1-windows'
archive = base / 'astc-local-rev1/packages' / (name + '.tar.xz')
extract = base / 'astc-native-extracted-rev1'
source = pathlib.Path('F:/JericoEngine/Dependencies/astc-encoder-f32d8d4dd9e06f8986cd5e10f12e199fc023f150')
provider = root / 'cmake/Tools/LocalPackages/ASTCEncoder'
expected_archive = '4eed3f722e8f4cca60bf14684f22fa351d0d59960ed518be04cce30c707949ca'
def sha(data): return hashlib.sha256(data).hexdigest()
def local(path):
    if not path.is_absolute() or str(path).startswith('\\\\'): raise ValueError('Nonlocal path')
    for candidate in (path, *path.parents):
        if candidate.exists() and candidate.stat().st_file_attributes & 0x400: raise ValueError('Reparse path: ' + str(candidate))
def safe(name):
    p = pathlib.PurePosixPath(name)
    if not name or p.is_absolute() or name != p.as_posix() or '\\' in name or ':' in name or any(x in ('', '.', '..') for x in name.split('/')): raise ValueError('Unsafe member: ' + name)
    for part in p.parts:
        if part.endswith(('.', ' ')) or re.match(r'^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)', part, re.I): raise ValueError('Windows path ambiguity: ' + name)
    return p
local(archive); local(extract)
if extract.exists(): raise ValueError('Fresh extraction required; preserve existing destination')
raw = archive.read_bytes()
if len(raw) != 1179220 or sha(raw) != expected_archive: raise ValueError('Archive pin mismatch')
sidecar = pathlib.Path(str(archive) + '.SHA256SUMS')
if sidecar.read_text('ascii').strip() != expected_archive + ' *' + archive.name: raise ValueError('Sidecar mismatch')
payload = {}; folded = {}; records = []
with tarfile.open(archive, 'r:xz') as tar:
    for member in tar:
        p = safe(member.name)
        if not member.isfile() or member.issparse() or member.pax_headers: raise ValueError('Unexpected type/PAX: ' + member.name)
        key = member.name.casefold()
        if key in folded: raise ValueError('Case/duplicate collision: ' + member.name)
        folded[key] = member.name
        data = tar.extractfile(member).read()
        if len(data) != member.size: raise ValueError('Truncated member: ' + member.name)
        payload[member.name] = data
        records.append(dict(path=member.name, bytes=len(data), sha256=sha(data)))
for path in payload:
    for parent in pathlib.PurePosixPath(path).parents:
        if str(parent) != '.' and str(parent).casefold() in folded: raise ValueError('File/directory collision: ' + path)
manifest = {}
for line in payload['SHA256SUMS'].decode('ascii').splitlines():
    digest, path = line.split(' *', 1); safe(path)
    if not re.fullmatch('[0-9a-f]{64}', digest) or path in manifest: raise ValueError('Invalid checksum manifest')
    manifest[path] = digest
if set(payload) != set(manifest) | {'SHA256SUMS'}: raise ValueError('Manifest coverage mismatch')
for path, digest in manifest.items():
    if sha(payload[path]) != digest: raise ValueError('Payload checksum mismatch: ' + path)
listed = [line.removeprefix('--add-file=') for line in archive.with_name(name + '.files').read_text('utf-8').splitlines()]
if sorted(listed) != sorted(payload): raise ValueError('File list mismatch')
info = json.loads(payload['PackageInfo.json'])
expected_info = dict(schemaVersion=1, name=name, version='3.2.0', license='Apache-2.0 AND MIT AND BSD-3-Clause AND Unlicense', source=dict(commit='f32d8d4dd9e06f8986cd5e10f12e199fc023f150', sha256='291c26fcf20b8e23a6eb1ad4dd75f5afecf79d316c0477c532df6d5693291403', url='https://codeload.github.com/ARM-software/astc-encoder/tar.gz/f32d8d4dd9e06f8986cd5e10f12e199fc023f150'))
if info != expected_info: raise ValueError('PackageInfo provenance mismatch')
recipe = json.loads(payload['astc-encoder/JERecipe.json'])
if recipe['historicalRecipeCommit'] != '90480ca41d15e1b20a28f90e2f996fd0e5a3a68c' or recipe['historicalPackageVersion'] != '3.2-rev2' or recipe['historicalBinaryHashMatch'] or recipe['sourceCommit'] != info['source']['commit'] or recipe['configurations'] != ['Release', 'Debug'] or recipe['isa'] != 'SSE4.1' or recipe['runtimeLibrary'] != 'MultiThreaded$<$<CONFIG:Debug>:Debug>DLL' or not recipe['fullCodec'] or not recipe['cliDebugTool'] or recipe['unitTests']: raise ValueError('Recipe contract mismatch')
notices = []
def original(path, original):
    data = original.read_bytes()
    if payload[path] != data: raise ValueError('Original/source witness mismatch: ' + path)
    notices.append(dict(packagePath=path, source=str(original), bytes=len(data), sha256=sha(data)))
original('astc-encoder/include/astcenc.h', source / 'Source/astcenc.h')
original('astc-encoder/LICENSE.txt', source / 'LICENSE.txt')
original('LICENSES/ASTC/LICENSE.txt', source / 'LICENSE.txt')
for file in sorted((source / 'Source').iterdir()):
    if file.suffix in ('.cpp', '.h'): original('LICENSES/ASTC/source-notices/' + file.name, file)
for file in ('LICENSE.txt', 'LICENSE_APACHE2.TXT', 'LICENSE_MIT.TXT'): original('LICENSES/O3DE/' + file, root / file)
for file in ('astc-encoder-3.2-rev1.patch', 'PatchHunks.json'): original('LICENSES/O3DE/recipe/' + file, provider / file)
for file in ('Findastc-encoder.cmake', 'NOTICE.txt', 'README.md'): original(file, provider / file)
original('JERequireLocalPath.cmake', root / 'cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake')
# Parse PE/COFF headers as data. No dumpbin, compiler or ASTC execution.
def pe(data):
    if data[:2] != b'MZ': raise ValueError('Expected PE image')
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    if data[offset:offset+4] != b'PE\0\0': raise ValueError('PE signature')
    machine = struct.unpack_from('<H', data, offset+4)[0]
    if machine != 0x8664: raise ValueError('PE not AMD64')
    return dict(machine='0x8664', peOffset=offset)
def coff(data):
    if data[:8] != b'!<arch>\n': raise ValueError('Expected COFF archive')
    offset = 8; objects = 0; names = []
    while offset < len(data):
        header = data[offset:offset+60]
        if len(header) != 60 or header[58:60] != b'`\n': raise ValueError('COFF member header')
        size = int(header[48:58]); name = header[:16].decode('ascii').strip(); body = data[offset+60:offset+60+size]
        if len(body) != size: raise ValueError('COFF member truncation')
        if name not in ('/', '//'):
            machine = struct.unpack_from('<H', body, 6 if body[:4] == b'\0\0\xff\xff' else 0)[0]
            if machine != 0x8664: raise ValueError('COFF object not AMD64: ' + name)
            objects += 1; names.append(name)
        offset += 60 + size + size % 2
    if offset != len(data) or objects == 0: raise ValueError('COFF archive length/objects')
    return dict(machine='0x8664', objectCount=objects, memberNames=names)
binaries = []
for config in ('Release', 'Debug'):
    for name, parser in (('astcenc-sse4.1.exe', pe), ('astcenc-sse4.1-static.lib', coff)):
        path = 'astc-encoder/bin/' + config + '/' + name
        binaries.append(dict(path=path, bytes=len(payload[path]), sha256=sha(payload[path]), structure=parser(payload[path])))
summary = dict(status='STATIC ARCHIVE AUDIT PASSED; NO CODE EXECUTED', archive=str(archive), archiveSHA256=expected_archive, archiveBytes=len(raw), members=len(payload), payloadBytes=sum(map(len,payload.values())), links=0, caseCollisions=0, unsafePaths=0, manifestEntries=len(manifest), packageInfo=info, recipe=recipe, binaries=binaries, originalNoticeWitnesses=notices, extractionRoot=str(extract), nativeExecuted=False)
(ev / 'archive-audit.json').write_text(json.dumps(summary, indent=2) + '\n', 'utf-8')
(ev / 'archive-members.json').write_text(json.dumps(records, indent=2) + '\n', 'utf-8')
extract.mkdir(parents=True, exist_ok=False)
for path, data in payload.items():
    dest = extract.joinpath(*pathlib.PurePosixPath(path).parts)
    dest.parent.mkdir(parents=True, exist_ok=True)
    with dest.open('xb') as stream: stream.write(data)
extraction_records = []
for entry in records:
    path = extract.joinpath(*pathlib.PurePosixPath(entry['path']).parts)
    if sha(path.read_bytes()) != entry['sha256'] or path.stat().st_size != entry['bytes']: raise ValueError('Extraction mismatch')
    extraction_records.append(dict(path=str(path), bytes=entry['bytes'], sha256=entry['sha256']))
(ev / 'extraction-manifest.json').write_text(json.dumps(extraction_records, indent=2) + '\n', 'utf-8')
print(json.dumps(dict(status=summary['status'], members=summary['members'], payloadBytes=summary['payloadBytes'], notices=len(notices), archiveSHA256=expected_archive, binaries=[dict(path=x['path'], bytes=x['bytes'], structure=x['structure']) for x in binaries]), indent=2))

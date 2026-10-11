# Static data-only adaptation of the accepted ASTC tar/lzma auditor.
# Never imports or executes package contents, compiler, or native SDK tools.
import hashlib
import json
import pathlib
import re
import struct
import tarfile

ROOT = pathlib.Path('F:/JericoEngine/JericoEngine')
BASE = ROOT / 'build/n0-validation/local-packages'
HERE = BASE / 'poly2tri-rev1-consumer-preparation'
NAME = 'poly2tri-7f0487a-je-rev1-windows'
ARCHIVE = BASE / 'poly2tri-local-rev1/packages' / (NAME + '.tar.xz')
EXTRACT = BASE / 'poly2tri-native-extracted-rev1'
SOURCE = pathlib.Path('F:/JericoEngine/Dependencies/poly2tri-7f0487a81124647f0a4037c4706ff414ce9fe22b')
PROVIDER = ROOT / 'cmake/Tools/LocalPackages/Poly2Tri'
HASH = 'f00af81d5124edb6f09b4a2de7a5c2855fa6d44f19fa7ac976ecb70fc79252a6'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def write(name, value):
    with (HERE / name).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')

def local(path):
    if not path.is_absolute() or str(path).startswith('\\\\'):
        raise ValueError('Nonlocal path')
    for candidate in (path, *path.parents):
        if candidate.exists() and candidate.stat().st_file_attributes & 0x400:
            raise ValueError('Reparse path: ' + str(candidate))

def safe(name):
    path = pathlib.PurePosixPath(name)
    if not name or path.is_absolute() or name != path.as_posix() or '\\' in name or ':' in name or any(x in ('', '.', '..') for x in name.split('/')):
        raise ValueError('Unsafe member: ' + name)
    for part in path.parts:
        if part.endswith(('.', ' ')) or re.match(r'^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)', part, re.I):
            raise ValueError('Windows path ambiguity: ' + name)
    return path

local(ARCHIVE)
local(EXTRACT)
if EXTRACT.exists():
    raise ValueError('Fresh extraction required')
raw = ARCHIVE.read_bytes()
if len(raw) != 208736 or sha(raw) != HASH:
    raise ValueError('Archive pin mismatch')
if pathlib.Path(str(ARCHIVE) + '.SHA256SUMS').read_text('ascii').strip() != HASH + ' *' + ARCHIVE.name:
    raise ValueError('Sidecar mismatch')
payload = {}
folded = set()
with tarfile.open(ARCHIVE, 'r:xz') as tar:
    for member in tar:
        safe(member.name)
        if not member.isfile() or member.issparse() or member.pax_headers or member.size > 32 * 1024 * 1024:
            raise ValueError('Unexpected type/PAX/size: ' + member.name)
        key = member.name.casefold()
        if key in folded:
            raise ValueError('Case/duplicate collision')
        folded.add(key)
        data = tar.extractfile(member).read()
        if len(data) != member.size:
            raise ValueError('Truncated member')
        payload[member.name] = data
for path in payload:
    for parent in pathlib.PurePosixPath(path).parents:
        if str(parent) != '.' and str(parent).casefold() in folded:
            raise ValueError('File/directory collision')
headers = ['poly2tri.h', 'common/shapes.h', 'common/utils.h', 'sweep/advancing_front.h', 'sweep/cdt.h', 'sweep/sweep.h', 'sweep/sweep_context.h']
sources = ['common/shapes.cc', 'sweep/advancing_front.cc', 'sweep/cdt.cc', 'sweep/sweep.cc', 'sweep/sweep_context.cc']
expected = {'Findpoly2tri.cmake', 'JERequireLocalPath.cmake', 'NOTICE.txt', 'README.md', 'Recipe.json', 'PackageInfo.json', 'SHA256SUMS', 'poly2tri/LICENSE', 'poly2tri/README.md', 'LICENSES/Poly2Tri/upstream-CMakeLists.txt', 'poly2tri/poly2tri/lib/poly2tri.lib', 'poly2tri/poly2tri/debug/lib/poly2tri.lib'}
expected.update('poly2tri/poly2tri/include/poly2tri/' + name for name in headers)
expected.update('LICENSES/Poly2Tri/compiled-source/' + name for name in sources)
expected.update('LICENSES/O3DE/' + name for name in ['LICENSE.txt', 'LICENSE_APACHE2.TXT', 'LICENSE_MIT.TXT'])
if set(payload) != expected:
    raise ValueError('Package contract membership mismatch')
manifest = {}
for line in payload['SHA256SUMS'].decode('ascii').splitlines():
    digest, path = line.split(' *', 1)
    safe(path)
    if not re.fullmatch('[0-9a-f]{64}', digest) or path in manifest:
        raise ValueError('Invalid checksum manifest')
    manifest[path] = digest
if set(payload) != set(manifest) | {'SHA256SUMS'}:
    raise ValueError('Manifest coverage mismatch')
for path, digest in manifest.items():
    if sha(payload[path]) != digest:
        raise ValueError('Payload checksum mismatch: ' + path)
listed = [line.removeprefix('--add-file=') for line in ARCHIVE.with_name(NAME + '.files').read_text('utf-8').splitlines()]
if sorted(listed) != sorted(payload):
    raise ValueError('File list mismatch')
info = json.loads(payload['PackageInfo.json'])
expected_info = dict(schemaVersion=1, name=NAME, version='2021-05-18', license='BSD-3-Clause', source=dict(commit='7f0487a81124647f0a4037c4706ff414ce9fe22b', sha256='680b7b505c15775ecdfb8868cb9f6f2d616b21fbb320158d056844add7cb9371', url='https://codeload.github.com/jhasse/poly2tri/tar.gz/7f0487a81124647f0a4037c4706ff414ce9fe22b'))
if info != expected_info:
    raise ValueError('PackageInfo provenance mismatch')
witnesses = []
def original(path, source):
    data = source.read_bytes()
    if payload[path] != data:
        raise ValueError('Original/source witness mismatch: ' + path)
    witnesses.append(dict(packagePath=path, source=str(source), bytes=len(data), sha256=sha(data)))
for name in headers:
    original('poly2tri/poly2tri/include/poly2tri/' + name, SOURCE / 'poly2tri' / name)
for name in sources:
    original('LICENSES/Poly2Tri/compiled-source/' + name, SOURCE / 'poly2tri' / name)
original('LICENSES/Poly2Tri/upstream-CMakeLists.txt', SOURCE / 'CMakeLists.txt')
original('poly2tri/LICENSE', SOURCE / 'LICENSE')
original('poly2tri/README.md', SOURCE / 'README.md')
for name in ['LICENSE.txt', 'LICENSE_APACHE2.TXT', 'LICENSE_MIT.TXT']:
    original('LICENSES/O3DE/' + name, ROOT / name)
for name in ['Findpoly2tri.cmake', 'NOTICE.txt', 'README.md', 'Recipe.json']:
    original(name, PROVIDER / name)
original('JERequireLocalPath.cmake', ROOT / 'cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake')
# Read COFF archive headers as data; no dumpbin/link/compiler invocation.
def coff(data):
    if data[:8] != b'!<arch>\n':
        raise ValueError('Expected COFF archive')
    offset = 8
    objects = 0
    while offset < len(data):
        header = data[offset:offset+60]
        if len(header) != 60 or header[58:60] != b'`\n':
            raise ValueError('COFF member header')
        size = int(header[48:58])
        name = header[:16].decode('ascii').strip()
        body = data[offset+60:offset+60+size]
        if len(body) != size:
            raise ValueError('COFF member truncation')
        if name not in ('/', '//'):
            if struct.unpack_from('<H', body, 6 if body[:4] == b'\0\0\xff\xff' else 0)[0] != 0x8664:
                raise ValueError('COFF object not AMD64')
            objects += 1
        offset += 60 + size + size % 2
    if offset != len(data) or objects != 5:
        raise ValueError('Expected five compiled COFF objects')
    return dict(machine='0x8664', objectCount=objects)
binaries = []
for config, path in [('Release', 'poly2tri/poly2tri/lib/poly2tri.lib'), ('Debug', 'poly2tri/poly2tri/debug/lib/poly2tri.lib')]:
    binaries.append(dict(configuration=config, path=path, bytes=len(payload[path]), sha256=sha(payload[path]), structure=coff(payload[path])))
records = [dict(path=path, bytes=len(data), sha256=sha(data)) for path, data in sorted(payload.items())]
summary = dict(status='STATIC_DATA_AUDIT_COMPLETED; NO_PACKAGE_CODE_EXECUTED', archive=str(ARCHIVE), archiveBytes=len(raw), archiveSHA256=HASH, members=len(payload), manifestEntries=len(manifest), payloadBytes=sum(map(len, payload.values())), links=0, caseCollisions=0, unsafePaths=0, packageInfo=info, binaries=binaries, originalWitnesses=witnesses, extractionRoot=str(EXTRACT), limits=['No CRT/symbol export/link/behavior certification from header inspection', 'No final engine/Editor/AP/cook/runtime acceptance'])
write('archive-audit.json', summary)
write('archive-members.json', records)
EXTRACT.mkdir(parents=True, exist_ok=False)
for path, data in payload.items():
    destination = EXTRACT.joinpath(*pathlib.PurePosixPath(path).parts)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open('xb') as stream:
        stream.write(data)
for record in records:
    destination = EXTRACT.joinpath(*pathlib.PurePosixPath(record['path']).parts)
    if sha(destination.read_bytes()) != record['sha256'] or destination.stat().st_size != record['bytes']:
        raise ValueError('Extraction mismatch')
write('extraction-manifest.json', dict(root=str(EXTRACT), archiveSHA256=HASH, entries=records, memberCount=len(records)))
print(json.dumps(dict(status=summary['status'], members=len(payload), manifestEntries=len(manifest), payloadBytes=summary['payloadBytes'], sourceWitnesses=len(witnesses), binaries=binaries)))

import hashlib
import json
import pathlib
import tarfile

base = pathlib.Path('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages')
archive = base / 'ispc-texcomp-rev1-build/packages/ISPCTexComp-36b80aa-je-rev1-windows.tar.xz'
payload = base / 'ispc-texcomp-rev1-build/packages/ISPCTexComp-36b80aa-je-rev1-windows.contents'
audit = base / 'ispc-texcomp-rev1-package-audit'
dest = audit / 'extracted/ISPCTexComp-36b80aa-je-rev1-windows'
pin = '8308677642330694AB1C63C74A7AAABF4A0A9633234F998B12E838CDA64008DF'
sha = lambda data: hashlib.sha256(data).hexdigest().upper()
def write_new(name, value):
    with (audit / name).open('x', encoding='utf-8', newline='\n') as out:
        json.dump(value, out, indent=2)
        out.write('\n')

def no_reparse(path):
    for part in (path,) + tuple(path.parents):
        if part.exists() and (part.lstat().st_file_attributes & 1024):
            raise RuntimeError('Reparse extraction ancestor')

no_reparse(dest)
if dest.exists():
    raise RuntimeError('Preserve existing extraction')
raw = archive.read_bytes()
if len(raw) != 491564 or sha(raw) != pin:
    raise RuntimeError('Produced archive pin mismatch')
files = {}
names = set()
with tarfile.open(archive, 'r:xz') as tar:
    for member in tar.getmembers():
        name = member.name
        parts = name.split('/')
        if not member.isreg() or member.linkname or name.startswith('/') or '\\' in name or ':' in name:
            raise RuntimeError('Nonregular/link/unsafe tar member')
        for part in parts:
            if not part or part in ('.', '..') or part.endswith(('.', ' ')) or any(c in part for c in '<>:"|?*'):
                raise RuntimeError('Unsafe path component')
            stem = part.split('.')[0].upper()
            if stem in ('CON', 'PRN', 'AUX', 'NUL') or stem in ['COM'+str(i) for i in range(1,10)] or stem in ['LPT'+str(i) for i in range(1,10)]:
                raise RuntimeError('Reserved Windows component')
        if name.casefold() in names:
            raise RuntimeError('Duplicate/case collision')
        names.add(name.casefold())
        if member.size < 0 or member.size > 50000000:
            raise RuntimeError('Unexpected member size')
        stream = tar.extractfile(member)
        if stream is None:
            raise RuntimeError('Missing regular data')
        data = stream.read()
        stream.close()
        if len(data) != member.size:
            raise RuntimeError('Truncated member')
        files[name] = data
for name in files:
    for parent in pathlib.PurePosixPath(name).parents:
        if str(parent).casefold() in names:
            raise RuntimeError('File/directory ancestor collision')
required = {'FindISPCTexComp.cmake', 'JERequireLocalPath.cmake', 'NOTICE.txt', 'README.md', 'PackageInfo.json', 'SHA256SUMS',
    'ISPCTexComp/JERecipe.json', 'ISPCTexComp/license.txt', 'ISPCTexComp/bin/ispc_texcomp.dll', 'ISPCTexComp/bin/ispc_texcomp.lib',
    'ISPCTexComp/include/ISPC/ispc_texcomp.h', 'LICENSES/ISPC-build-tool/LICENSE.txt', 'LICENSES/ISPC-build-tool/third-party-programs.txt',
    'LICENSES/ISPCTexComp/source/ispc_texcomp.cpp', 'LICENSES/ISPCTexComp/source/ispc_texcomp.h',
    'LICENSES/ISPCTexComp/source/ispc_texcomp_astc.cpp', 'LICENSES/ISPCTexComp/source/kernel.ispc', 'LICENSES/ISPCTexComp/source/kernel_astc.ispc',
    'LICENSES/O3DE/LICENSE.txt', 'LICENSES/O3DE/LICENSE_APACHE2.TXT', 'LICENSES/O3DE/LICENSE_MIT.TXT'}
if set(files) != required:
    raise RuntimeError('Original layout/payload closure differs')
sums = {}
for line in files['SHA256SUMS'].decode('utf-8').splitlines():
    digest, name = line.split(' *', 1)
    if len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest) or name in sums:
        raise RuntimeError('Malformed/duplicate content manifest')
    sums[name] = digest.upper()
if set(sums) != set(files) - {'SHA256SUMS'}:
    raise RuntimeError('Content manifest coverage differs')
records = []
for name, data in files.items():
    digest = sha(data)
    if name != 'SHA256SUMS' and digest != sums[name]:
        raise RuntimeError('Archive content manifest mismatch')
    if data != (payload / name).read_bytes():
        raise RuntimeError('Archive differs from producer payload')
    records.append(dict(path=name, type='regular', bytes=len(data), sha256=digest))
info = json.loads(files['PackageInfo.json'])
if info['name'] != 'ISPCTexComp-36b80aa-je-rev1-windows' or info['version'] != '36b80aa' or info['license'] != 'MIT':
    raise RuntimeError('Package identity/license differs')
if info['source'] != dict(commit='36b80aac50ea401a9adde44d86712c2241b2acfd', sha256='029373e6ec5b10d7e29eefdc5a231fb86c79b76a4a39c58c2727c6729de28ab7',
        url='https://codeload.github.com/GameTechDev/ISPCTextureCompressor/tar.gz/36b80aac50ea401a9adde44d86712c2241b2acfd'):
    raise RuntimeError('Source provenance differs')
source = pathlib.Path('F:/JericoEngine/Dependencies/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd')
tool = pathlib.Path('F:/JericoEngine/Dependencies/tools/ispc-v1.16.1-windows')
notice_pairs = [('ISPCTexComp/license.txt', source / 'license.txt'),
    ('ISPCTexComp/include/ISPC/ispc_texcomp.h', source / 'ispc_texcomp/ispc_texcomp.h'),
    ('LICENSES/ISPC-build-tool/LICENSE.txt', tool / 'LICENSE.txt'),
    ('LICENSES/ISPC-build-tool/third-party-programs.txt', tool / 'third-party-programs.txt')]
for name in ('ispc_texcomp.cpp','ispc_texcomp.h','ispc_texcomp_astc.cpp','kernel.ispc','kernel_astc.ispc'):
    notice_pairs.append(('LICENSES/ISPCTexComp/source/'+name, source / 'ispc_texcomp' / name))
for package_name, original in notice_pairs:
    if files[package_name] != original.read_bytes():
        raise RuntimeError('License/API/source notice bytes differ')
write_new('archive-audit-before-extraction.json', dict(archive=str(archive), sha256=pin, bytes=len(raw), files=len(records),
    links=0, caseCollisions=0, allRegular=True, contentManifestCovered=20, producerPayloadByteExact=True,
    licenseAndSourceNoticesExact=len(notice_pairs), toolExecutableRedistributed=False, packageInfo=info, entries=records))
dest.mkdir(parents=True, exist_ok=False)
for name, data in files.items():
    target = dest / name
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('xb') as out:
        out.write(data)
    if target.read_bytes() != data:
        raise RuntimeError('Extracted bytes differ')
write_new('extracted-package-manifest.json', dict(root=str(dest), files=len(records), source='audited produced archive', entries=records))
write_new('package-audit-summary.json', dict(status='ARCHIVE_AUDITED_FRESH_EXTRACTED_NOT_CONSUMER_VERIFIED',
    archiveSHA256=pin, archiveBytes=len(raw), files=len(records), root=str(dest), productTooling=False,
    note='No acquired code executed. CPython external stdlib data-only tar reader. No build/runtime acceptance.'))
print(json.dumps(dict(files=len(records), root=str(dest), archiveSHA256=pin, status='DATA_AUDIT_ONLY'), indent=2))
"""Static acceptance of the official archive, including its declared CRLF export rules."""
import hashlib, io, json, tarfile
from pathlib import Path, PurePosixPath
OUT=Path(__file__).resolve().parent
ARCHIVE=Path('F:/JericoEngine/Dependencies/archives/lz4-v1.9.4.tar.gz')
SNAPSHOT=Path('F:/JericoEngine/Dependencies/lz4-1.9.4')
SHA='0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b'
PIN='5ff839680134437dbf4678f3d0c7b371d84f4964'
data=(OUT/'source-candidate.tar.gz').read_bytes()
if hashlib.sha256(data).hexdigest()!=SHA or len(data)!=354063: raise ValueError('Candidate changed')
for path in (ARCHIVE,SNAPSHOT):
    if path.exists() or any(parent.is_symlink() for parent in path.parents): raise ValueError('Destination exists or symlink')
tree=json.loads((OUT/'tree.json').read_text())
expected={row['path']:row for row in tree['tree'] if row['type']=='blob'}
if tree.get('truncated') or any(row['type']=='commit' or row['mode']=='120000' for row in tree['tree']): raise ValueError('Incomplete tree or links')
diff=json.loads((OUT/'archive-difference.json').read_text())
differences={row['path']:row for row in diff['differences']}
if len(differences)!=33 or not all(row['normalized_lf_equal'] for row in differences.values()): raise ValueError('Unexpected differences')
rows,payloads,seen,cases=[],{},set(),set()
with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
    members=archive.getmembers()
    if len(members)>2000 or sum(member.size for member in members)>20971520: raise ValueError('Tar bounds')
    attributes=archive.extractfile('lz4-1.9.4/.gitattributes').read()
    if not all(rule in attributes for rule in (b'*.sln text eol=crlf',b'*.vcxproj* text eol=crlf',b'*.rc text eol=crlf')): raise ValueError('CRLF declaration absent')
    for member in members:
        name=member.name.rstrip('/'); parts=PurePosixPath(name).parts
        if not parts or parts[0]!='lz4-1.9.4' or any(part in ('','.','..') for part in parts) or '\\' in name or ':' in name or name.startswith('/'):
            raise ValueError('Unsafe archive path')
        if name in seen or name.casefold() in cases: raise ValueError('Path/case collision')
        seen.add(name); cases.add(name.casefold())
        if not member.isdir() and not member.isfile(): raise ValueError('Special type/link refused')
        relative='/'.join(parts[1:])
        row={'path':name,'relative_path':relative,'type':'directory' if member.isdir() else 'regular','bytes':member.size,'mode':member.mode}
        if member.isfile():
            if relative not in expected: raise ValueError('Unexpected file')
            body=archive.extractfile(member).read()
            blob=hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()
            if relative in differences:
                if not relative.endswith(('.sln','.vcxproj','.rc')): raise ValueError('Difference is not declared CRLF file')
                raw=(OUT/'difference-raw'/relative).read_bytes()
                normalized=body.replace(b'\r\n',b'\n')
                if normalized!=raw or hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()!=expected[relative]['sha']:
                    raise ValueError('CRLF-only correspondence failed')
                row.update(git_blob_matches=False,declared_crlf_export=True,normalized_git_blob_sha1=expected[relative]['sha'])
            elif blob!=expected[relative]['sha'] or len(body)!=expected[relative]['size']:
                raise ValueError('Byte-exact source differs: '+relative)
            else: row.update(git_blob_matches=True,declared_crlf_export=False)
            row.update(sha256=hashlib.sha256(body).hexdigest(),archive_git_blob_sha1=blob)
            payloads[relative]=body
        rows.append(row)
if set(payloads)!=set(expected): raise ValueError('Commit files omitted')
def save(name,value):
    with (OUT/name).open('x',encoding='utf-8') as file: file.write(json.dumps(value,indent=2)+'\n')
with ARCHIVE.open('xb') as file: file.write(data)
save('source-acquisition.json',{'url':diff['url'],'commit':PIN,'archive':str(ARCHIVE),'bytes':len(data),'sha256':SHA,
                              'receipts':diff['receipts'][:2],'pin_evidence':'official tag and full Git tree; 33 declared CRLF exports, all compiled library bytes exact'})
save('source-tar-audit.json',{'sha256':SHA,'commit':PIN,'members':len(rows),'regular_files':len(payloads),'regular_bytes':sum(map(len,payloads.values())),
                            'links':0,'declared_crlf_exports':33,'compiled_library_git_blobs_byte_exact':True,'all_commit_paths_present':True,'entries':rows})
SNAPSHOT.mkdir()
for relative,body in payloads.items():
    target=SNAPSHOT/relative; target.parent.mkdir(parents=True,exist_ok=True)
    with target.open('xb') as file: file.write(body)
    if hashlib.sha256(target.read_bytes()).hexdigest()!=hashlib.sha256(body).hexdigest(): raise ValueError('Extraction SHA mismatch')
save('source-extraction.json',{'snapshot':str(SNAPSHOT),'files':len(payloads),'bytes':sum(map(len,payloads.values())),
                              'all_sha256_checked':True,'source_patch':False,'configure_executed':False,'entries':[row for row in rows if row['type']=='regular']})
print(json.dumps({'archive_bytes':len(data),'sha256':SHA,'members':len(rows),'files':len(payloads),'source_bytes':sum(map(len,payloads.values())),'links':0,'CRLF_exports':33}))

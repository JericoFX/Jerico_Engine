"""One explicit parent-approved archive transfer, followed by static tar/blob inspection."""
import datetime
import hashlib
import io
import json
import tarfile
import urllib.error
import urllib.request
from pathlib import Path, PurePosixPath
OUT = Path(__file__).resolve().parent
URL = 'https://codeload.github.com/google/benchmark/tar.gz/refs/tags/v1.7.0'
SHA = '3aff99169fa8bdee356eaa1f691e835a6e57b1efeadb8a0f9f228531158246ac'
ARCHIVE = Path('F:/JericoEngine/Dependencies/archives/googlebenchmark-v1.7.0.tar.gz')
SNAPSHOT = Path('F:/JericoEngine/Dependencies/googlebenchmark-1.7.0')
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
receipts = []
def save(name, value):
    with (OUT/name).open('x',encoding='utf-8') as file:
        file.write(json.dumps(value,indent=2)+'\n')
def transfer(method):
    request = urllib.request.Request(URL,method=method,headers={'User-Agent':'JE-parent-authorized-googlebenchmark-acquisition'})
    try:
        response = opener.open(request,timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        receipt = {'url':URL,'method':method,'status':response.code,'final_url':response.geturl(),
                   'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'authorization':False,'automatic_redirects':False,
                   'headers':{key:response.headers.get(key) for key in ('Content-Type','Content-Length','ETag','Location')}}
        receipts.append(receipt)
        if response.code != 200 or response.geturl()!=URL or response.headers.get('Location') or response.headers.get('Content-Type')!='application/x-gzip':
            raise ValueError('Direct archive destination refused')
        data = response.read(10485761) if method=='GET' else b''
        if len(data)>10485760:
            raise ValueError('Archive exceeds bounded acquisition')
        return data
for path in (ARCHIVE,SNAPSHOT):
    if path.exists():
        raise ValueError('Refusing existing archive or snapshot output: '+str(path))
    for ancestor in path.parents:
        if ancestor.exists() and ancestor.is_symlink():
            raise ValueError('Symlink ancestor refused')
try:
    transfer('HEAD')
    data = transfer('GET')
    actual = hashlib.sha256(data).hexdigest()
    if actual!=SHA:
        raise ValueError('Archive SHA differs from current consumer exact pin')
    ARCHIVE.parent.mkdir(parents=True,exist_ok=True)
    with ARCHIVE.open('xb') as file:
        file.write(data)
    save('source-acquisition.json',{'url':URL,'archive':str(ARCHIVE),'bytes':len(data),'sha256':actual,'receipts':receipts})
except Exception as error:
    save('source-acquisition-failure.json',{'error':str(error),'receipts':receipts})
    raise

tree = json.loads((OUT/'tree.json').read_text())
expected = {row['path']:row for row in tree['tree'] if row['type']=='blob'}
if any(row['type']=='commit' for row in tree['tree']):
    raise ValueError('Submodule not included in source closure')
rows, payloads, seen, case_paths = [], {}, set(), set()
with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
    members = archive.getmembers()
    if len(members)>2000 or sum(member.size for member in members)>20971520:
        raise ValueError('Tar exceeds bounded source audit')
    for member in members:
        name = member.name.rstrip('/')
        parts = PurePosixPath(name).parts
        if not parts or parts[0]!='benchmark-1.7.0' or any(part in ('','.','..') for part in parts) or '\\' in name or ':' in name or name.startswith('/'):
            raise ValueError('Unsafe archive path')
        if name in seen or name.casefold() in case_paths:
            raise ValueError('Tar path/case collision')
        seen.add(name)
        case_paths.add(name.casefold())
        if not member.isdir() and not member.isfile():
            raise ValueError('Non-directory/non-regular tar member refused')
        relative = '/'.join(parts[1:])
        row = {'path':name,'relative_path':relative,'type':'directory' if member.isdir() else 'regular','bytes':member.size,'mode':member.mode}
        if member.isfile():
            if relative not in expected:
                raise ValueError('Tar file absent from exact commit tree: '+relative)
            body = archive.extractfile(member).read()
            blob_sha = hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()
            if blob_sha != expected[relative]['sha'] or len(body)!=expected[relative]['size']:
                raise ValueError('Tar body differs from exact Git blob: '+relative)
            row.update(sha256=hashlib.sha256(body).hexdigest(),git_blob_sha1=blob_sha,git_blob_matches=True)
            payloads[relative] = body
        rows.append(row)
if set(payloads)!=set(expected):
    raise ValueError('Archive omits commit source files')
save('source-tar-audit.json',{'archive_sha256':SHA,'commit':'361e8d1cfe0c6c36d30b39f1b61302ece5507320',
    'members':len(rows),'regular_files':len(payloads),'regular_bytes':sum(len(body) for body in payloads.values()),
    'links':0,'all_commit_blob_matches':True,'license_sha256':hashlib.sha256(payloads['LICENSE']).hexdigest(),'entries':rows})
SNAPSHOT.mkdir()
for relative, body in payloads.items():
    target = SNAPSHOT/relative
    target.parent.mkdir(parents=True,exist_ok=True)
    with target.open('xb') as file:
        file.write(body)
    if hashlib.sha256(target.read_bytes()).hexdigest()!=hashlib.sha256(body).hexdigest():
        raise ValueError('Extracted source hash differs')
save('source-extraction.json',{'snapshot':str(SNAPSHOT),'files':len(payloads),'bytes':sum(len(body) for body in payloads.values()),
    'all_sha256_checked':True,'source_patch':False,'configure_executed':False,'entries':[row for row in rows if row['type']=='regular']})
print(json.dumps({'archive_bytes':len(data),'sha256':SHA,'members':len(rows),'files':len(payloads),'source_bytes':sum(len(body) for body in payloads.values()),'links':0}))

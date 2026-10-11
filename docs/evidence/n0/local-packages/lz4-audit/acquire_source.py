"""Manual authorized LZ4 archive acquisition, followed by full static Git-blob audit."""
import datetime, hashlib, io, json, tarfile, urllib.error, urllib.request
from pathlib import Path, PurePosixPath
OUT=Path(__file__).resolve().parent
URL='https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4'
PIN='5ff839680134437dbf4678f3d0c7b371d84f4964'
ARCHIVE=Path('F:/JericoEngine/Dependencies/archives/lz4-v1.9.4.tar.gz')
SNAPSHOT=Path('F:/JericoEngine/Dependencies/lz4-1.9.4')
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs): return None
opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),NoRedirect)
receipts=[]
def save(name,value):
    with (OUT/name).open('x',encoding='utf-8') as file: file.write(json.dumps(value,indent=2)+'\n')
def transfer(method):
    request=urllib.request.Request(URL,method=method,headers={'User-Agent':'JE-parent-authorized-lz4-acquisition'})
    try: response=opener.open(request,timeout=30)
    except urllib.error.HTTPError as error: response=error
    with response:
        receipt={'url':URL,'method':method,'status':response.code,'final_url':response.geturl(),
                 'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'authorization':False,'automatic_redirects':False,
                 'headers':{key:response.headers.get(key) for key in ('Content-Type','Content-Length','ETag','Location')}}
        receipts.append(receipt)
        if response.code!=200 or response.geturl()!=URL or response.headers.get('Location') or response.headers.get('Content-Type')!='application/x-gzip':
            raise ValueError('Direct official archive destination refused')
        data=response.read(10485761) if method=='GET' else b''
        if len(data)>10485760: raise ValueError('Bounded archive exceeded')
        return data
for path in (ARCHIVE,SNAPSHOT):
    if path.exists(): raise ValueError('Existing destination refused: '+str(path))
    if any(ancestor.is_symlink() for ancestor in path.parents): raise ValueError('Symlink ancestor refused')
try:
    ref=json.loads((OUT/'tag-reference.json').read_text())['object']
    if ref['type']=='tag': ref=json.loads((OUT/'annotated-tag.json').read_text())['object']
    if ref['type']!='commit' or ref['sha']!=PIN: raise ValueError('Tag pin differs from audited commit')
    transfer('HEAD')
    data=transfer('GET')
    sha=hashlib.sha256(data).hexdigest()
    tree=json.loads((OUT/'tree.json').read_text())
    expected={row['path']:row for row in tree['tree'] if row['type']=='blob'}
    if tree.get('truncated') or any(row['type']=='commit' or row['mode']=='120000' for row in tree['tree']):
        raise ValueError('Truncated tree/submodules/symlinks refused')
    rows,payloads,seen,cases=[],{},set(),set()
    with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
        members=archive.getmembers()
        if len(members)>2000 or sum(member.size for member in members)>20971520: raise ValueError('Tar bound exceeded')
        for member in members:
            name=member.name.rstrip('/'); parts=PurePosixPath(name).parts
            if not parts or parts[0]!='lz4-1.9.4' or any(part in ('','.','..') for part in parts) or '\\' in name or ':' in name or name.startswith('/'):
                raise ValueError('Unsafe tar path')
            if name in seen or name.casefold() in cases: raise ValueError('Tar path/case collision')
            seen.add(name); cases.add(name.casefold())
            if not member.isdir() and not member.isfile(): raise ValueError('Tar links/special types refused')
            relative='/'.join(parts[1:])
            row={'path':name,'relative_path':relative,'type':'directory' if member.isdir() else 'regular','bytes':member.size,'mode':member.mode}
            if member.isfile():
                if relative not in expected: raise ValueError('Unexpected source path')
                body=archive.extractfile(member).read()
                blob=hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()
                if blob!=expected[relative]['sha'] or len(body)!=expected[relative]['size']: raise ValueError('Source differs from pinned Git blob: '+relative)
                row.update(sha256=hashlib.sha256(body).hexdigest(),git_blob_sha1=blob,git_blob_matches=True)
                payloads[relative]=body
            rows.append(row)
    if set(payloads)!=set(expected): raise ValueError('Archive omits commit files')
    # The historical binary package SHA is deliberately not a source-archive hash.
    ARCHIVE.parent.mkdir(parents=True,exist_ok=True)
    with ARCHIVE.open('xb') as file: file.write(data)
    save('source-acquisition.json',{'url':URL,'commit':PIN,'archive':str(ARCHIVE),'bytes':len(data),'sha256':sha,
                                  'pin_evidence':'official tag and every source body matches exact commit Git tree','receipts':receipts})
    save('source-tar-audit.json',{'archive_sha256':sha,'commit':PIN,'members':len(rows),'regular_files':len(payloads),
                                'regular_bytes':sum(len(body) for body in payloads.values()),'links':0,'all_commit_blob_matches':True,'entries':rows})
    SNAPSHOT.mkdir()
    for relative,body in payloads.items():
        target=SNAPSHOT/relative; target.parent.mkdir(parents=True,exist_ok=True)
        with target.open('xb') as file: file.write(body)
        if hashlib.sha256(target.read_bytes()).hexdigest()!=hashlib.sha256(body).hexdigest(): raise ValueError('Extraction SHA mismatch')
    save('source-extraction.json',{'snapshot':str(SNAPSHOT),'files':len(payloads),'bytes':sum(len(body) for body in payloads.values()),
                                   'all_sha256_checked':True,'source_patch':False,'configure_executed':False,'entries':[row for row in rows if row['type']=='regular']})
    print(json.dumps({'archive_bytes':len(data),'sha256':sha,'commit':PIN,'members':len(rows),'files':len(payloads),'source_bytes':sum(len(body) for body in payloads.values()),'links':0}))
except Exception as error:
    save('source-acquisition-failure.json',{'error':str(error),'receipts':receipts})
    raise

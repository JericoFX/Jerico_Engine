"""Inspect the exact official archive/commit discrepancy; never extract or execute."""
import hashlib, io, json, tarfile, urllib.request
from pathlib import Path
OUT=Path(__file__).resolve().parent
URL='https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4'
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs): return None
opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),NoRedirect)
receipts=[]
def read(url,method='GET'):
    with opener.open(urllib.request.Request(url,method=method,headers={'User-Agent':'JE-parent-authorized-lz4-byte-diagnostic'}),timeout=30) as response:
        if response.code!=200 or response.geturl()!=url or response.headers.get('Location'): raise ValueError('Destination refused')
        body=response.read(10485761) if method=='GET' else b''
        if len(body)>10485760: raise ValueError('Bound exceeded')
        receipts.append({'url':url,'method':method,'status':response.code,'final_url':response.geturl(),'automatic_redirects':False,'authorization':False,
                         'headers':dict(response.headers),'body_bytes':len(body),'body_sha256':hashlib.sha256(body).hexdigest() if method=='GET' else None})
        return body
read(URL,'HEAD')
data=read(URL)
with (OUT/'source-candidate.tar.gz').open('xb') as file: file.write(data)
tree=json.loads((OUT/'tree.json').read_text())
rows={row['path']:row for row in tree['tree'] if row['type']=='blob'}
diff=[]
with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
    for member in archive.getmembers():
        if not member.isfile(): continue
        relative=member.name.split('/',1)[1]
        body=archive.extractfile(member).read()
        blob=hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()
        if blob!=rows[relative]['sha']:
            raw=read('https://raw.githubusercontent.com/lz4/lz4/5ff839680134437dbf4678f3d0c7b371d84f4964/'+relative)
            rawblob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
            if rawblob!=rows[relative]['sha']: raise ValueError('Raw does not match Git blob')
            diff.append({'path':relative,'archive_bytes':len(body),'archive_sha256':hashlib.sha256(body).hexdigest(),'git_bytes':len(raw),
                         'git_blob_sha1':rawblob,'archive_git_blob_sha1':blob,'normalized_lf_equal':body.replace(b'\r\n',b'\n')==raw.replace(b'\r\n',b'\n'),
                         'archive_crlf':body.count(b'\r\n'),'git_crlf':raw.count(b'\r\n')})
            target=OUT/'difference-raw'/relative; target.parent.mkdir(parents=True,exist_ok=True)
            with target.open('xb') as file: file.write(raw)
    attributes=archive.extractfile('lz4-1.9.4/.gitattributes').read()
with (OUT/'archive-gitattributes.txt').open('xb') as file: file.write(attributes)
result={'url':URL,'archive_bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'differences':diff,'receipts':receipts}
with (OUT/'archive-difference.json').open('x',encoding='utf-8') as file: file.write(json.dumps(result,indent=2)+'\n')
print(json.dumps({'bytes':len(data),'sha256':result['sha256'],'differences':diff}))

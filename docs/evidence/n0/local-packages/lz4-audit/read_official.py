"""Read parent-authorized official LZ4 metadata/text only, with no redirects/auth."""
import datetime
import hashlib
import json
import urllib.error
import urllib.request
from pathlib import Path

OUT = Path(__file__).resolve().parent
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
receipts = []

def read(url, method='GET'):
    request = urllib.request.Request(url, method=method, headers={'User-Agent':'JE-parent-authorized-lz4-text-audit'})
    try:
        response = opener.open(request, timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        data = response.read(2097153) if method == 'GET' else b''
        row = {'url':url, 'method':method, 'status':response.code, 'final_url':response.geturl(),
               'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(), 'authorization':False,
               'automatic_redirects':False, 'bytes':len(data),
               'sha256':hashlib.sha256(data).hexdigest() if method == 'GET' else None,
               'headers':{key:response.headers.get(key) for key in ('Content-Type','Content-Length','ETag','Location')}}
        receipts.append(row)
        if len(data)>2097152 or response.code!=200 or response.geturl()!=url or response.headers.get('Location'):
            raise ValueError('Official destination refused: '+str(row))
        return data

def save(name, data):
    path = OUT/name
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as file:
        file.write(data)

def metadata(name, url):
    data = read(url)
    save(name, data)
    return json.loads(data)

try:
    ref = metadata('tag-reference.json','https://api.github.com/repos/lz4/lz4/git/ref/tags/v1.9.4')
    obj = ref['object']
    if obj['type']=='tag':
        obj = metadata('annotated-tag.json','https://api.github.com/repos/lz4/lz4/git/tags/'+obj['sha'])['object']
    if obj['type']!='commit':
        raise ValueError('Tag does not resolve to commit')
    pin = obj['sha']
    metadata('commit.json','https://api.github.com/repos/lz4/lz4/commits/'+pin)
    tree = metadata('tree.json','https://api.github.com/repos/lz4/lz4/git/trees/'+pin+'?recursive=1')
    if tree.get('truncated'):
        raise ValueError('Tree truncated')
    selected = [row for row in tree['tree'] if row['type']=='blob' and
                (row['path'].endswith('.cmake') or row['path'].endswith('CMakeLists.txt') or
                 row['path'].endswith('.vcxproj') or row['path'] in ('LICENSE','lib/LICENSE','README.md','.gitmodules','lib/lz4.h','lib/lz4frame.h'))]
    inputs = []
    for row in selected:
        body = read('https://raw.githubusercontent.com/lz4/lz4/'+pin+'/'+row['path'])
        body.decode('utf-8')
        blob = hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()
        if len(body)!=row['size'] or blob!=row['sha']:
            raise ValueError('Text differs from commit blob')
        save('upstream/'+row['path'],body)
        inputs.append({'path':row['path'],'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest(),'git_blob_sha1':blob})
    read('https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4','HEAD')
    save('text-inputs.json',(json.dumps(inputs,indent=2)+'\n').encode())
    print(json.dumps({'commit':pin,'texts':len(inputs),'archive_GET':False}))
finally:
    save('destination-receipts.json',(json.dumps(receipts,indent=2)+'\n').encode())

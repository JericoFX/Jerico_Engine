"""Static CMake input closure texts; no configure, compile or archive transfer."""
import hashlib
import json
import urllib.request
from pathlib import Path
OUT = Path(__file__).resolve().parent
PIN = '361e8d1cfe0c6c36d30b39f1b61302ece5507320'
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
tree = json.loads((OUT/'tree.json').read_text())
selected = [row for row in tree['tree'] if row['type'] == 'blob' and
            (row['path'].endswith('.cmake.in') or row['path'] == 'cmake/benchmark.pc.in' or
             (row['path'].startswith('cmake/') and row['path'].endswith('.cpp')))]
receipts = []
for row in selected:
    url = 'https://raw.githubusercontent.com/google/benchmark/'+PIN+'/'+row['path']
    request = urllib.request.Request(url, headers={'User-Agent':'JE-parent-authorized-source-text-audit'})
    with opener.open(request, timeout=30) as response:
        if response.code != 200 or response.geturl() != url or response.headers.get('Location'):
            raise ValueError('Official text destination differs')
        data = response.read(1048577)
        data.decode('utf-8')
        if len(data) != row['size'] or hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest() != row['sha']:
            raise ValueError('Commit text blob differs')
        target = OUT/'upstream'/row['path']
        with target.open('xb') as file:
            file.write(data)
        receipts.append({'url':url,'status':response.code,'final_url':response.geturl(),'authorization':False,
                         'automatic_redirects':False,'path':row['path'],'bytes':len(data),
                         'git_blob_sha1':row['sha'],'sha256':hashlib.sha256(data).hexdigest()})
(OUT/'additional-text-inputs.json').write_text(json.dumps(receipts,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'additional_texts':len(receipts),'archive_get':False}))

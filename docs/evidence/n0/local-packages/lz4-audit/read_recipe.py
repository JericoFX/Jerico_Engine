"""Official O3DE recipe text audit, fixed historical commit; no recipe execution."""
import datetime, hashlib, json, urllib.request
from pathlib import Path
OUT = Path(__file__).resolve().parent
PIN = '70f0b7b5257bf7cf935bf1cfd270e1b53ccead18'
REPO = 'o3de/3p-package-source'
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
receipts = []
def read(url):
    with opener.open(urllib.request.Request(url, headers={'User-Agent':'JE-parent-authorized-lz4-recipe-audit'}),timeout=30) as response:
        body = response.read(4194305)
        if len(body)>4194304 or response.code!=200 or response.geturl()!=url or response.headers.get('Location'):
            raise ValueError('Official recipe destination refused')
        receipts.append({'url':url,'status':response.code,'final_url':response.geturl(),'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
                         'authorization':False,'automatic_redirects':False,'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest()})
        return body
def save(name, body):
    path=OUT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('xb') as file:
        file.write(body)
try:
    body = read('https://api.github.com/repos/'+REPO+'/git/trees/'+PIN+'?recursive=1')
    save('recipe-tree.json',body)
    tree=json.loads(body)
    if tree.get('truncated'):
        raise ValueError('Recipe tree truncated')
    selected=[row for row in tree['tree'] if row['type']=='blob' and ('lz4' in row['path'].lower() or row['path'] in ('LICENSE','LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT'))]
    for row in selected:
        body=read('https://raw.githubusercontent.com/'+REPO+'/'+PIN+'/'+row['path'])
        body.decode('utf-8')
        if len(body)!=row['size'] or hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()!=row['sha']:
            raise ValueError('Recipe bytes differ from pinned Git blob')
        save('recipe/'+row['path'],body)
    print(json.dumps({'repository':REPO,'commit':PIN,'recipe_paths':[row['path'] for row in selected]}))
finally:
    save('recipe-destination-receipts.json',(json.dumps(receipts,indent=2)+'\n').encode())

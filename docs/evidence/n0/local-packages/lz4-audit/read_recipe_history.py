"""Read original build driver and LZ4 recipe history as text; never execute it."""
import hashlib, json, urllib.request
from pathlib import Path
OUT=Path(__file__).resolve().parent
PIN='70f0b7b5257bf7cf935bf1cfd270e1b53ccead18'
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs): return None
opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),NoRedirect)
receipts=[]
def read(url):
    with opener.open(urllib.request.Request(url,headers={'User-Agent':'JE-parent-authorized-lz4-history-audit'}),timeout=30) as response:
        body=response.read(1048577)
        if len(body)>1048576 or response.code!=200 or response.geturl()!=url: raise ValueError('Official text refused')
        receipts.append({'url':url,'status':response.code,'automatic_redirects':False,'authorization':False,'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest()})
        return body
def save(name,body):
    path=OUT/name; path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('xb') as file: file.write(body)
try:
    tree=json.loads((OUT/'recipe-tree.json').read_text())
    rows={row['path']:row for row in tree['tree']}
    for name in ('Scripts/extras/pull_and_build_from_git.py','package_build_list_host_windows.json'):
        body=read('https://raw.githubusercontent.com/o3de/3p-package-source/'+PIN+'/'+name)
        if hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()!=rows[name]['sha']: raise ValueError('Git blob mismatch')
        save('recipe/'+name,body)
    history=read('https://api.github.com/repos/o3de/3p-package-source/commits?path=package-system/lz4/build_config.json&sha='+PIN+'&per_page=20')
    save('recipe-config-history.json',history)
    commits=json.loads(history)
    configs=[]
    for commit in commits[:8]:
        sha=commit['sha']
        body=read('https://raw.githubusercontent.com/o3de/3p-package-source/'+sha+'/package-system/lz4/build_config.json')
        save('history/'+sha+'-build_config.json',body)
        version=json.loads(body)['package_version']
        configs.append({'commit':sha,'package_version':version,'message':commit['commit']['message']})
        if version=='1.9.4-rev1':
            body=read('https://raw.githubusercontent.com/o3de/3p-package-source/'+sha+'/package-system/lz4/Findlz4.cmake')
            save('history/'+sha+'-Findlz4.cmake',body)
            break
    save('recipe-version-history.json',(json.dumps(configs,indent=2)+'\n').encode())
    print(json.dumps(configs))
finally:
    save('recipe-history-receipts.json',(json.dumps(receipts,indent=2)+'\n').encode())

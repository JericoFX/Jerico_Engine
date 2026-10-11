"""Official release/PR metadata and candidate HEADs; no alternate binary GET."""
import base64
import hashlib
import json
import urllib.error
import urllib.request
from pathlib import Path
OUT=Path(__file__).resolve().parent
PATH='Code/Editor/splashscreen_background.png'
OID='92290da114ef3cccd5c8afad1bc0de4b9060a522b08b4218e4d211d2793ecd8c'
BLOB='1b65fb4e2f01c3d1566095816ff78858ab8b8265'
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs): return None
opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),NoRedirect)
receipts=[]
def request(url,method='GET'):
    req=urllib.request.Request(url,method=method,headers={'User-Agent':'JE-parent-authorized-splash-release-metadata-audit'})
    try: response=opener.open(req,timeout=30)
    except urllib.error.HTTPError as error: response=error
    with response:
        row={'url':url,'method':method,'status':response.code,'final_url':response.geturl(),
             'authorization':False,'automatic_redirects':False,
             'headers':{name:response.headers.get(name) for name in ('Content-Length','Content-Type','ETag','Location')}}
        receipts.append(row)
        if method=='HEAD': return row
        if response.code!=200 or response.geturl()!=url: return None
        data=response.read(1048577)
        if len(data)>1048576: raise ValueError('Bounded metadata too large')
        row['metadata_sha256']=hashlib.sha256(data).hexdigest()
        return json.loads(data)
tags=request('https://api.github.com/repos/o3de/o3de/tags?per_page=50')
pr=request('https://api.github.com/repos/o3de/o3de/pulls/19779/commits?per_page=20')
blob=request('https://api.github.com/repos/o3de/o3de/git/blobs/'+BLOB)
if blob['encoding']!='base64': raise ValueError('Unexpected pointer blob metadata')
pointer=base64.b64decode(blob['content'])
if not pointer.startswith(b'version https://git-lfs.github.com/spec/v1\n') or ('oid sha256:'+OID+'\nsize 1708521\n').encode() not in pointer:
    raise ValueError('History blob is not the exact requested pointer')
if hashlib.sha1(b'blob '+str(len(pointer)).encode()+b'\0'+pointer).hexdigest()!=BLOB:
    raise ValueError('History pointer Git blob differs')
candidates,detail_rows=[],[]
for commit in pr or []:
    detail=request('https://api.github.com/repos/o3de/o3de/commits/'+commit['sha'])
    files=[row for row in (detail or {}).get('files',[]) if row['filename']==PATH]
    detail_rows.append({'commit':commit['sha'],'files':files})
    if any(row['sha']==BLOB for row in files): candidates.append({'commit':commit['sha'],'reason':'original PR19779 exact pointer blob'})
for tag in tags or []:
    if '26' in tag['name'] and ('05' in tag['name'] or '50' in tag['name']):
        meta=request('https://api.github.com/repos/o3de/o3de/contents/'+PATH+'?ref='+tag['commit']['sha'])
        if meta and meta.get('sha')==BLOB:
            candidates.append({'commit':tag['commit']['sha'],'tag':tag['name'],'reason':'official release tag exact pointer blob'})
for candidate in candidates:
    url='https://media.githubusercontent.com/media/o3de/o3de/'+candidate['commit']+'/'+PATH
    head=request(url,'HEAD')
    candidate.update(url=url,head=head,head_matches=head['status']==200 and head['headers']['Content-Length']=='1708521' and head['headers']['ETag']=='"'+OID+'"',payload_get=False)
report={'path':PATH,'expected_sha256':OID,'expected_bytes':1708521,'pointer_git_blob_sha1':BLOB,
        'pointer_git_blob_verified':True,'pointer_text':pointer.decode(),'tags_metadata':tags,'pr19779':detail_rows,
        'candidates':candidates,'receipts':receipts,'payload_get':False,'full_payload_sha_verified':False}
(OUT/'splash-release-history-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'tags':[row['name'] for row in tags or []],'pr_commits':len(pr or []),'candidates':candidates,'payload_get':False}))

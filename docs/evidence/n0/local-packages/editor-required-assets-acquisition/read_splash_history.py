"""Official metadata/history and HEAD only; never GETs alternate splash payload."""
import datetime
import json
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
OUT = Path(__file__).resolve().parent
PATH = 'Code/Editor/splashscreen_background.png'
OID = '92290da114ef3cccd5c8afad1bc0de4b9060a522b08b4218e4d211d2793ecd8c'
SIZE = 1708521
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
receipts = []
def request(url, method='GET'):
    req = urllib.request.Request(url,method=method,headers={'User-Agent':'JE-parent-authorized-original-asset-history-audit'})
    try:
        response = opener.open(req,timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        row = {'url':url,'method':method,'status':response.code,'final_url':response.geturl(),
               'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'authorization':False,'automatic_redirects':False,
               'headers':{key:response.headers.get(key) for key in ('Content-Length','Content-Type','ETag','Location')}}
        receipts.append(row)
        if method == 'HEAD':
            return row
        if response.code != 200 or response.geturl() != url:
            raise ValueError('Official metadata refused')
        data = response.read(1048577)
        if len(data) > 1048576:
            raise ValueError('History metadata unexpectedly large')
        return json.loads(data)

history = request('https://api.github.com/repos/o3de/o3de/commits?path='+urllib.parse.quote(PATH,safe='')+'&per_page=20')
(OUT/'splash-path-history.json').write_text(json.dumps(history,indent=2)+'\n',encoding='utf-8')
details, candidates = [], []
for row in history:
    detail = request('https://api.github.com/repos/o3de/o3de/commits/'+row['sha'])
    affected = [entry for entry in detail.get('files',[]) if entry['filename']==PATH]
    details.append({'commit':row['sha'],'message':detail['commit']['message'],'files':affected})
    for entry in affected:
        patch = entry.get('patch','')
        if '+'+'oid sha256:'+OID in patch:
            for sha in [row['sha']]:
                url = 'https://media.githubusercontent.com/media/o3de/o3de/'+sha+'/'+urllib.parse.quote(PATH)
                head = request(url,'HEAD')
                candidates.append({'commit':sha,'url':url,'pointer_patch_oid_matches':True,'expected_sha256':OID,'expected_bytes':SIZE,
                    'head':head,'head_matches':head['status']==200 and head['headers']['Content-Length']==str(SIZE) and head['headers']['ETag']=='"'+OID+'"',
                    'payload_get':False,'full_payload_sha256_verified':False})
(OUT/'splash-history-audit.json').write_text(json.dumps({'path':PATH,'expected_sha256':OID,'expected_bytes':SIZE,
    'history':details,'candidates':candidates,'receipts':receipts,'payload_get':False,
    'limits':'No alternate GET authorized. Candidate HEAD does not replace full SHA verification or parent approval.'},indent=2)+'\n',encoding='utf-8')
print(json.dumps({'history_commits':len(history),'head_candidates':len(candidates),'head_matches':sum(row['head_matches'] for row in candidates),'payload_get':False}))

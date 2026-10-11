"""Parent-authorized text/metadata audit; never downloads the source archive."""
import datetime
import hashlib
import json
import urllib.error
import urllib.request
from pathlib import Path

OUT = Path(__file__).resolve().parent
PIN = '361e8d1cfe0c6c36d30b39f1b61302ece5507320'
class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
receipts = []
def read(url, method='GET'):
    request = urllib.request.Request(url, method=method, headers={'User-Agent': 'JE-parent-authorized-source-text-audit'})
    try:
        response = opener.open(request, timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        row = {'url': url, 'method': method, 'status': response.code, 'final_url': response.geturl(),
               'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'authorization': False,
               'automatic_redirects': False,
               'headers': {key: response.headers.get(key) for key in ['Content-Length','Content-Type','ETag','Location']}}
        data = b'' if method == 'HEAD' else response.read(1048577)
        if len(data) > 1048576:
            raise ValueError('Text metadata unexpectedly exceeds bound')
        row['body_sha256'] = hashlib.sha256(data).hexdigest() if method == 'GET' else None
        row['body_bytes'] = len(data)
        receipts.append(row)
        if method == 'GET' and (response.code != 200 or response.geturl() != url):
            raise ValueError('Official text metadata refused: '+str(row))
        return data
def save(name, data):
    path = OUT/name
    with path.open('xb') as file:
        file.write(data)
def metadata(name, url):
    data = read(url)
    save(name, data)
    return json.loads(data)

try:
    read('https://github.com/google/benchmark/archive/refs/tags/v1.7.0.tar.gz', 'HEAD')
    read('https://codeload.github.com/google/benchmark/tar.gz/refs/tags/v1.7.0', 'HEAD')
    ref = metadata('tag-reference.json', 'https://api.github.com/repos/google/benchmark/git/ref/tags/v1.7.0')
    obj = ref['object']
    if obj['type'] == 'tag':
        tag = metadata('annotated-tag.json', 'https://api.github.com/repos/google/benchmark/git/tags/'+obj['sha'])
        obj = tag['object']
    if obj['type'] != 'commit' or obj['sha'] != PIN:
        raise ValueError('Official tag differs from consumer commit pin')
    commit = metadata('commit.json', 'https://api.github.com/repos/google/benchmark/commits/'+PIN)
    tree = metadata('tree.json', 'https://api.github.com/repos/google/benchmark/git/trees/'+PIN+'?recursive=1')
    if tree.get('truncated'):
        raise ValueError('Tree metadata truncated')
    selected = [row for row in tree['tree'] if row['type'] == 'blob' and
                (row['path'].endswith('.cmake') or row['path'].endswith('CMakeLists.txt') or
                 row['path'] in ('LICENSE','AUTHORS','CONTRIBUTORS','NOTICE','.gitmodules'))]
    (OUT/'upstream').mkdir()
    inputs = []
    for row in selected:
        data = read('https://raw.githubusercontent.com/google/benchmark/'+PIN+'/'+row['path'])
        data.decode('utf-8')
        if len(data) != row['size'] or hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest() != row['sha']:
            raise ValueError('Official text bytes differ from commit tree blob: '+row['path'])
        path = OUT/'upstream'/row['path']
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as file:
            file.write(data)
        inputs.append({'path': row['path'], 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'git_blob_sha1': row['sha']})
    save('text-inputs.json', (json.dumps(inputs, indent=2)+'\n').encode())
finally:
    save('destination-receipts.json', (json.dumps(receipts, indent=2)+'\n').encode())
print(json.dumps({'tag_commit': PIN, 'text_inputs': len(selected), 'source_archive_get': False}))

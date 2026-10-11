"""Explicit parent-approved original asset acquisition; no engine code or payload execution."""
import argparse
import datetime
import hashlib
import json
import shutil
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path('F:/JericoEngine/JericoEngine')
MANIFEST = ROOT / 'build/n0-validation/local-packages/editor-required-assets-audit/acquisition-review-manifest.json'
OUT = ROOT / 'build/n0-validation/local-packages/editor-required-assets-acquisition'
SNAPSHOT = Path('F:/JericoEngine/Dependencies/assets/n0-original-editor')
PIN = '7982d0db8511004f342af03f5e73b2dab510974b7be37508a0c640bfc0dd95a4'

def sha(data):
    return hashlib.sha256(data).hexdigest()
def save(path, data):
    path.write_text(json.dumps(data, indent=2)+'\n', encoding='utf-8')
def safe_relative(value):
    parsed = urllib.parse.urlsplit(value)
    parts = Path(value).parts
    if parsed.scheme or value.startswith(('/', '\\')) or '..' in parts or ':' in value:
        raise ValueError('Unsafe snapshot relative path')
    return value

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, msg, headers, newurl):
        return None

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect)
def transfer(item, method):
    url = item['payload_candidate_url']
    parsed = urllib.parse.urlsplit(url)
    expected_path = '/media/o3de/o3de/'+item['github_commit']+'/'+urllib.parse.quote(safe_relative(item['path']))
    if parsed.scheme != 'https' or parsed.netloc != 'media.githubusercontent.com' or parsed.path != expected_path or parsed.query or parsed.fragment:
        raise ValueError('Destination differs from the approved official pinned path')
    request = urllib.request.Request(url, method=method, headers={'User-Agent': 'JE-authorized-original-asset-acquisition'})
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    try:
        response = opener.open(request, timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        receipt = {'url': url, 'method': method, 'utc': started, 'status': response.code,
                   'final_url': response.geturl(), 'authorization': False, 'automatic_redirects': False,
                   'headers': {key: response.headers.get(key) for key in ['Content-Type','Content-Length','ETag','Location']}}
        if response.code != 200 or response.geturl() != url or receipt['headers']['Location']:
            raise ValueError('Direct destination refused: '+json.dumps(receipt))
        if receipt['headers']['Content-Length'] != str(item['payload_bytes']):
            raise ValueError('Payload content length differs from approved pointer')
        if receipt['headers']['ETag'] != '"'+item['payload_sha256']+'"':
            raise ValueError('Payload ETag differs from approved OID')
        body = response.read(item['payload_bytes']+1) if method == 'GET' else b''
    return receipt, body

arguments = argparse.ArgumentParser()
arguments.add_argument('--start', type=int, default=0)
arguments.add_argument('--count', type=int, default=30)
args = arguments.parse_args()
raw = MANIFEST.read_bytes()
if sha(raw) != PIN:
    raise ValueError('Approved manifest changed')
manifest = json.loads(raw)
items = manifest['items']
if args.start < 0 or args.count <= 0 or args.start >= len(items):
    raise ValueError('Invalid approved item range')
state_path = OUT / 'snapshot-state.json'
if not state_path.exists():
    if SNAPSHOT.exists():
        raise ValueError('Snapshot exists without acquisition ownership receipt')
    SNAPSHOT.mkdir(parents=True)
    (OUT / 'payloads').mkdir()
    (OUT / 'receipts').mkdir()
    save(state_path, {'snapshot': str(SNAPSHOT), 'manifest_sha256': PIN, 'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()})
elif json.loads(state_path.read_text())['manifest_sha256'] != PIN:
    raise ValueError('Snapshot ownership receipt disagrees')

font_recovery = {row['descriptor']: row for row in manifest['font_relationships']}
for index in range(args.start, min(args.start+args.count, len(items))):
    item = items[index]
    destination = SNAPSHOT / safe_relative(item['path'])
    receipt_path = OUT / 'receipts' / ('%03d.json' % index)
    if receipt_path.exists() or destination.exists():
        raise ValueError('Refusing overwrite/repeated acquisition: '+item['path'])
    original = ROOT / item['path']
    if sha(original.read_bytes()) != item['pointer_sha256']:
        raise ValueError('Original checkout pointer changed: '+item['path'])
    receipt = {'index': index, 'source_path': item['path'], 'expected_sha256': item['payload_sha256'],
               'expected_bytes': item['payload_bytes'], 'checkout_materialized': False}
    try:
        head, _ = transfer(item, 'HEAD')
        receipt['head'] = head
        payload = OUT / 'payloads' / item['payload_sha256']
        if item['path'] in font_recovery:
            relation = font_recovery[item['path']]
            source = ROOT / relation['xml']
            data = source.read_bytes().replace(b'\r\n', b'\n')
            receipt['local_recovery'] = {'xml': relation['xml'], 'xml_sha256': sha(source.read_bytes()), 'transform': 'CRLF to LF only'}
        elif payload.exists():
            data = payload.read_bytes()
            receipt['deduplicated_from_payload_sha256'] = item['payload_sha256']
        else:
            get, data = transfer(item, 'GET')
            receipt['get'] = get
        if len(data) != item['payload_bytes'] or sha(data) != item['payload_sha256']:
            raise ValueError('Full payload SHA256/size mismatch')
        if data.startswith(b'version https://git-lfs.github.com/spec/v1'):
            raise ValueError('Refusing another LFS pointer as a binary payload')
        if not payload.exists():
            with payload.open('xb') as output:
                output.write(data)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open('xb') as output:
            output.write(data)
        receipt.update(actual_sha256=sha(data), actual_bytes=len(data), destination=str(destination), status='acquired and byte verified')
        save(receipt_path, receipt)
    except Exception as error:
        receipt.update(status='failed', error=str(error))
        save(OUT / ('failure-%03d.json' % index), receipt)
        raise

if args.start+args.count >= len(items):
    for notice in manifest['license_and_notice_sources']:
        relative = safe_relative(notice['path'])
        source = ROOT / relative
        data = source.read_bytes()
        if sha(data) != notice['sha256'] or len(data) != notice['bytes']:
            raise ValueError('Original notice changed')
        target = SNAPSHOT / 'notices' / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as output:
            output.write(data)
    shutil.copyfile(MANIFEST, SNAPSHOT / 'acquisition-review-manifest.json')
    receipts = [json.loads(path.read_text()) for path in sorted((OUT/'receipts').glob('*.json'))]
    failures = [json.loads(path.read_text()) for path in sorted(OUT.glob('failure-*.json'))]
    save(OUT/'acquisition-summary.json', {'status': ('complete' if len(receipts) == len(items) else 'partial with unavailable approved paths')+' static acquisition; engine validation not performed',
        'manifest_sha256': PIN, 'snapshot': str(SNAPSHOT), 'paths': len(receipts),
        'expected_paths': len(items), 'failures': failures,
        'logical_payload_bytes': sum(row['actual_bytes'] for row in receipts),
        'unique_payloads': len(list((OUT/'payloads').iterdir())),
        'get_count': sum('get' in row for row in receipts), 'head_count': sum('head' in row for row in receipts),
        'local_font_recoveries': sum('local_recovery' in row for row in receipts),
        'deduplicated_paths': sum('deduplicated_from_payload_sha256' in row for row in receipts),
        'checkout_materialized': False, 'payload_execution': False, 'notices': manifest['license_and_notice_sources']})
print(json.dumps({'completed_range': [args.start, min(args.start+args.count, len(items))], 'total': len(items)}))

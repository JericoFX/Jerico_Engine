import hashlib, json, pathlib, sys
root=pathlib.Path(r'F:\JericoEngine\JericoEngine')
base=root/'build/n0-validation/local-packages'
repo_root=pathlib.Path(r'F:\JericoEngine\JericoEngine')
freeze_path=base/'source-freeze-root-googlebenchmark-assets.json'
fixture_path=base/'source-freeze-editor-fixture-resources-parent.json'
bench_path=base/'googlebenchmark-audit/source-freeze.json'
cache_root=base/'root-package-cache'
mimalloc_root=pathlib.Path(r'F:\JericoEngine\Dependencies\mimalloc-3.5.2')
rapid_root=pathlib.Path(r'F:\JericoEngine\Dependencies\rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f')
archive_root=pathlib.Path(r'F:\JericoEngine\Dependencies\archives')
expected={'root_freeze':'41e00769ad46731946342c2d73038fb8f9236e27eb639144da71953665ac518e','fixture_freeze':'cc763794f88632255b5c563ea47d0759bf344df63d107a46ef2aad18c86d430e','bench_freeze':'c1bb254893203f0dbbe3cf1ecd2cadc8572527aec0b3fb8cbdab6fb00ef22b86','requirements':'d2891893d22b227c5d85bd4f1d2ebcca63a67c767be64a79cf7121100ddc6a6e'}
def digest(path):
    h=hashlib.sha256(); n=0
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk); n+=len(chunk)
    return n,h.hexdigest()
def verify_manifest_file(path, inputs, pathkey='path'):
    results=[]
    for item in inputs:
        p=pathlib.Path(item[pathkey]); n,h=digest(p) if p.is_file() else (None,None)
        want=item['sha256'].lower()
        results.append({'path':str(p),'bytes_expected':item.get('bytes'),'bytes_actual':n,'sha256_expected':want,'sha256_actual':h,'match':p.is_file() and h==want and (item.get('bytes') is None or item['bytes']==n)})
    return results

def tree(path):
    files=[]
    for p in sorted((x for x in path.rglob('*') if x.is_file()),key=lambda x:str(x).casefold()):
        n,h=digest(p); files.append({'path':str(p.relative_to(path)).replace('\\','/'),'bytes':n,'sha256':h})
    return {'root':str(path),'count':len(files),'bytes':sum(x['bytes'] for x in files),'files':files}
rootfreeze=json.loads(freeze_path.read_text(encoding='utf-8')); fixture=json.loads(fixture_path.read_text(encoding='utf-8')); bench=json.loads(bench_path.read_text(encoding='utf-8'))
root_files=verify_manifest_file(freeze_path,rootfreeze)
fixture_files=verify_manifest_file(fixture_path,fixture)
bench_files=verify_manifest_file(bench_path,bench['inputs'])
# Only the seven explicitly recorded package/source archives and the assigned benchmark archive.
archive_expected={
'cpython-49965601d6afedafe47cc85556d99b7a24981051.tar.gz':'12c564214c8a3b109d294143cb439e8bccb1cf6be677b52e98c5e2c41941ea44',
'Lua-5.4.4-je-rev1-windows.tar.xz':'d3962b7b84f7fb484f02f639f631db5613f30a6922953ce41ce8344e4b57cbeb',
'zstd-1.3.5-je-rev1-windows.tar.xz':'672edc4a41e31c6f45675b84a314cf1a31caa75428c543ea67a27253d9588793',
'qt-6.11.2-rev1-windows.tar.xz':'f09edc180df0ded92827279c475167be13cbb517de182a5b10ec1337e68dd834',
'pyside6-6.11.2-py3.10-rev1-windows.tar.xz':'cd0272eb5ce46a3437132344a4a6b7022e3aee152da888963cdd9d025cbdf506',
'mimalloc-3.5.2-source.tar.gz':'db5c4aaaf356edfeb1aa236b3a052fe3c67d01419db36613f783999341ed5619',
'rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f.tar.gz':'2d2601a82d2d3b7e143a3c8d43ef616671391034bc46891a9816b79cf2d3e7a8',
'googlebenchmark-v1.7.0.tar.gz':'3aff99169fa8bdee356eaa1f691e835a6e57b1efeadb8a0f9f228531158246ac'}
archives=[]
for name,want in archive_expected.items():
    p=archive_root/name; n,h=digest(p) if p.is_file() else (None,None)
    archives.append({'path':str(p),'bytes':n,'sha256':h,'expected_sha256':want,'match':p.is_file() and h==want})
cache=tree(cache_root); mimalloc=tree(mimalloc_root); rapidjson=tree(rapid_root)
reqpath=root/'python/requirements.txt'; reqbytes,reqsha=digest(reqpath)
result={'stage':sys.argv[1] if len(sys.argv)>1 else 'unspecified','freeze_files':{'root_path':str(freeze_path),'root_sha256':hashlib.sha256(freeze_path.read_bytes()).hexdigest(),'root_expected_sha256':expected['root_freeze'],'root_count':len(rootfreeze),'root_asset_prefix_count':sum(x['path'].startswith('Assets/') for x in rootfreeze),'root_checks':root_files,'fixture_path':str(fixture_path),'fixture_sha256':hashlib.sha256(fixture_path.read_bytes()).hexdigest(),'fixture_expected_sha256':expected['fixture_freeze'],'fixture_count':len(fixture),'fixture_checks':fixture_files,'bench_path':str(bench_path),'bench_sha256':hashlib.sha256(bench_path.read_bytes()).hexdigest(),'bench_expected_sha256':expected['bench_freeze'],'bench_count':len(bench['inputs']),'bench_checks':bench_files},'requirements':{'path':str(reqpath),'bytes':reqbytes,'sha256':reqsha,'expected_sha256':expected['requirements'],'match':reqsha==expected['requirements']},'root_package_cache':cache,'mimalloc_source':mimalloc,'rapidjson_source':rapidjson,'archives':archives}
result['all_pinned_inputs_match']=result['freeze_files']['root_sha256']==expected['root_freeze'] and len(rootfreeze)==303 and all(x['match'] for x in root_files) and result['freeze_files']['fixture_sha256']==expected['fixture_freeze'] and len(fixture)==22 and all(x['match'] for x in fixture_files) and result['freeze_files']['bench_sha256']==expected['bench_freeze'] and len(bench['inputs'])==174 and all(x['match'] for x in bench_files) and result['requirements']['match'] and cache['count']==60 and mimalloc['count']==382 and rapidjson['count']==313 and all(x['match'] for x in archives)
print(json.dumps(result,indent=2))
if not result['all_pinned_inputs_match']: raise SystemExit(1)

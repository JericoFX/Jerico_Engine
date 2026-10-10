import hashlib, json, pathlib, tarfile, collections
archive = pathlib.Path('F:/JericoEngine/Dependencies/archives/mimalloc-3.5.2-source.tar.gz')
expected = 'db5c4aaaf356edfeb1aa236b3a052fe3c67d01419db36613f783999341ed5619'
if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
    raise RuntimeError('archive hash mismatch')
rows, paths = [], set()
reserved = {'con','prn','aux','nul',*(f'com{i}' for i in range(1,10)),*(f'lpt{i}' for i in range(1,10))}
with tarfile.open(archive, 'r:gz') as tf:
    for m in tf.getmembers():
        p = pathlib.PurePosixPath(m.name)
        if p.is_absolute() or not p.parts or p.parts[0] != 'mimalloc-3.5.2' or any(x in ('','.','..') or ':' in x or '\\' in x or x.endswith((' ','.')) or x.split('.')[0].casefold() in reserved for x in p.parts):
            raise RuntimeError(f'unsafe archive path: {m.name}')
        key = '/'.join(p.parts).casefold()
        if key in paths: raise RuntimeError(f'Windows path collision: {m.name}')
        paths.add(key)
        if not (m.isfile() or m.isdir()): raise RuntimeError(f'unsupported archive type: {m.name} {m.type!r}')
        row = {'path':m.name,'type':'file' if m.isfile() else 'directory','bytes':m.size}
        if m.isfile(): row['sha256'] = hashlib.sha256(tf.extractfile(m).read()).hexdigest()
        rows.append(row)
    license_text = tf.extractfile('mimalloc-3.5.2/LICENSE').read().decode('utf-8')
result = {'archive':str(archive),'sha256':expected,'source_root':'mimalloc-3.5.2','members':len(rows),'files':sum(r['type']=='file' for r in rows),'payload_bytes':sum(r['bytes'] for r in rows),'links':0,'license':license_text,'entries':rows,'extracted':False}
out = pathlib.Path('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mimalloc-source-tar-audit.json')
out.write_text(json.dumps(result,indent=2)+'\n', encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='entries'},indent=2))
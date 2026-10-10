import hashlib, json, pathlib, tarfile, collections
archive = pathlib.Path('F:/JericoEngine/Dependencies/archives/lua-5.4.4-source.tar.gz')
expected = '2ff89bbea22a7c8b910d6b006f5ba4559edcf468a25537cedf9988ed90f6b450'
if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
    raise RuntimeError('archive hash mismatch')
rows, paths = [], set()
reserved = {'con','prn','aux','nul',*(f'com{i}' for i in range(1,10)),*(f'lpt{i}' for i in range(1,10))}
with tarfile.open(archive, 'r:gz') as tf:
    for m in tf.getmembers():
        p = pathlib.PurePosixPath(m.name)
        if p.is_absolute() or not p.parts or p.parts[0] != 'lua-5.4.4' or any(x in ('','.','..') or ':' in x or '\\' in x or x.endswith((' ','.')) or x.split('.')[0].casefold() in reserved for x in p.parts):
            raise RuntimeError(f'unsafe archive path: {m.name}')
        key = '/'.join(p.parts).casefold()
        if key in paths: raise RuntimeError(f'Windows path collision: {m.name}')
        paths.add(key)
        if not (m.isfile() or m.isdir()): raise RuntimeError(f'unsupported archive type: {m.name} {m.type!r}')
        row = {'path':m.name,'type':'file' if m.isfile() else 'directory','bytes':m.size}
        if m.isfile(): row['sha256'] = hashlib.sha256(tf.extractfile(m).read()).hexdigest()
        rows.append(row)
    license_text = tf.extractfile('lua-5.4.4/lua.h').read().decode('utf-8').rsplit('Copyright (C)',1)[-1]
result = {'archive':str(archive),'sha256':expected,'source_root':'lua-5.4.4','members':len(rows),'files':sum(r['type']=='file' for r in rows),'payload_bytes':sum(r['bytes'] for r in rows),'links':0,'license':license_text,'entries':rows,'extracted':False}
out = pathlib.Path('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lua-source-tar-audit.json')
out.write_text(json.dumps(result,indent=2)+'\n', encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='entries'},indent=2))
import hashlib, json, pathlib, tarfile, shutil
root = pathlib.Path('F:/JericoEngine/Dependencies')
archive = root/'archives/mimalloc-3.5.2-source.tar.gz'
audit_path = pathlib.Path('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mimalloc-source-tar-audit.json')
audit = json.loads(audit_path.read_text(encoding='utf-8'))
if hashlib.sha256(archive.read_bytes()).hexdigest() != audit['sha256']: raise RuntimeError('archive drift')
destination = root/audit['source_root']
if destination.exists(): raise RuntimeError('immutable source destination already exists')
with tarfile.open(archive,'r:gz') as tf:
    for m in tf.getmembers():
        target = root.joinpath(*pathlib.PurePosixPath(m.name).parts)
        if root not in target.resolve().parents: raise RuntimeError('destination containment failed')
        if m.isdir(): target.mkdir(parents=True,exist_ok=True)
        else:
            target.parent.mkdir(parents=True,exist_ok=True)
            with tf.extractfile(m) as source, target.open('xb') as output: shutil.copyfileobj(source,output)
for row in audit['entries']:
    target = root.joinpath(*pathlib.PurePosixPath(row['path']).parts)
    if row['type']=='file' and hashlib.sha256(target.read_bytes()).hexdigest()!=row['sha256']: raise RuntimeError('extracted source integrity failed')
result={'archive_sha256':audit['sha256'],'source_root':str(destination),'files':audit['files'],'bytes':audit['payload_bytes'],'hash_failures':0,'script_executed':False}
pathlib.Path('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mimalloc-source-extraction.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result))
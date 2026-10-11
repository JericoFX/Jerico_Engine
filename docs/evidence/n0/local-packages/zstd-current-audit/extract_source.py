import hashlib,json,os,pathlib,tarfile
out=pathlib.Path(r'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\zstd-current-audit')
archive=pathlib.Path(r'F:\JericoEngine\Dependencies\archives\zstd-90ae50224d15e8dbcb9fa26b9be096366733db8e.tar.gz')
root=pathlib.Path(r'F:\JericoEngine\Dependencies\zstd-90ae50224d15e8dbcb9fa26b9be096366733db8e')
audit=json.loads((out/'tar-audit.json').read_text(encoding='utf-8'))
records=json.loads((out/'tar-members.json').read_text(encoding='utf-8'))
if root.exists(): raise SystemExit('Source exists; refusing overwrite')
if not audit['safe_for_regular_file_extraction'] or hashlib.sha256(archive.read_bytes()).hexdigest()!=audit['archive_sha256']: raise SystemExit('Unaudited archive')
for ancestor in (root.parent,*root.parent.parents):
 if ancestor.stat().st_file_attributes & 0x400: raise SystemExit('Reparse ancestor '+str(ancestor))
record_by_path={x['path']:x for x in records}
for r in records:
 p=pathlib.PurePosixPath(r['path'])
 for parent in p.parents:
  if str(parent) in record_by_path and record_by_path[str(parent)]['type']!='directory': raise SystemExit('File ancestor collision')
root.mkdir()
with tarfile.open(archive,'r:gz') as tar:
 for m in tar.getmembers():
  parts=pathlib.PurePosixPath(m.name).parts[1:]
  target=root.joinpath(*parts)
  if os.path.commonpath((str(root),str(target.resolve())))!=str(root): raise SystemExit('Escaping target')
  if m.isdir(): target.mkdir(parents=True,exist_ok=True)
  elif m.isfile():
   target.parent.mkdir(parents=True,exist_ok=True)
   with target.open('xb') as f: f.write(tar.extractfile(m).read())
  else: raise SystemExit('Unexpected member type')
manifest=[]
for r in records:
 if r['type']!='file': continue
 p=root.joinpath(*pathlib.PurePosixPath(r['path']).parts[1:])
 actual=hashlib.sha256(p.read_bytes()).hexdigest()
 if actual!=r['sha256'] or p.stat().st_size!=r['bytes']: raise SystemExit('Extracted byte mismatch '+str(p))
 manifest.append({'path':str(p.relative_to(root)).replace('\\','/'),'bytes':p.stat().st_size,'sha256':actual})
actual_files=[p for p in root.rglob('*') if p.is_file()]
if len(actual_files)!=len(manifest): raise SystemExit('Extra/missing files')
reparse=[str(p) for p in root.rglob('*') if p.stat().st_file_attributes & 0x400]
if reparse: raise SystemExit('Extracted reparse path')
summary={'root':str(root),'archive_sha256':audit['archive_sha256'],'files':len(manifest),'payload_bytes':sum(x['bytes'] for x in manifest),'all_hashes_match':True,'reparse_paths':reparse,'extraction':'regular files/directories only into new root; no upstream scripts executed','snapshot_policy':'immutable input; subsequent builds must use external binary dirs and preserve these source hashes'}
(out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
(out/'source-extraction.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
print(json.dumps(summary,indent=2))

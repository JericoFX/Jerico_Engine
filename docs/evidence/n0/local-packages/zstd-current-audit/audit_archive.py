import hashlib,json,pathlib,re,tarfile
archive=pathlib.Path(r'F:\JericoEngine\Dependencies\archives\zstd-90ae50224d15e8dbcb9fa26b9be096366733db8e.tar.gz')
out=pathlib.Path(r'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\zstd-current-audit')
expected='dfb594dc41e79c1304ef2c9232746c048e4837578c1ca88cf0e9b83c8cc36457'
if hashlib.sha256(archive.read_bytes()).hexdigest()!=expected: raise SystemExit('Pin mismatch')
items=[]; problems=[]; seen={}; selected=[]; total=0
reserved=re.compile(r'^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)',re.I)
with tarfile.open(archive,'r:gz') as tar:
 for m in tar.getmembers():
  p=pathlib.PurePosixPath(m.name)
  key='/'.join(p.parts).casefold()
  errors=[]
  if p.is_absolute() or not p.parts or p.parts[0]!='zstd-90ae50224d15e8dbcb9fa26b9be096366733db8e': errors.append('unexpected root/absolute')
  if '\\' in m.name or any(x in ('.','..') or ':' in x or x.rstrip(' .')!=x or reserved.match(x) for x in m.name.rstrip('/').split('/')): errors.append('unsafe Windows path')
  if key in seen: errors.append('casefold path collision with '+seen[key])
  seen[key]=m.name
  kind='file' if m.isfile() else 'directory' if m.isdir() else 'symlink' if m.issym() else 'hardlink' if m.islnk() else 'other'
  if kind not in ('file','directory'): errors.append('non-regular member')
  record={'path':m.name,'type':kind,'bytes':m.size,'mode':oct(m.mode),'link_target':m.linkname,'errors':errors}
  if errors: problems.append(record)
  if m.isfile():
   data=tar.extractfile(m).read()
   if len(data)!=m.size: raise SystemExit('size mismatch '+m.name)
   record['sha256']=hashlib.sha256(data).hexdigest();total+=len(data)
   name=p.name.lower()
   is_cmake=name.startswith('cmakelists.txt') or '.cmake' in name
   is_license=bool(re.search(r'license|licence|copying|copyright|notice',name))
   if not errors and (is_cmake or is_license or name in ('.gitmodules','.git','readme.md','makefile')):
    target=out/'inspected-text'/pathlib.Path(*p.parts[1:])
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(data)
    selected.append({'path':m.name,'is_cmake':is_cmake,'is_license':is_license,'sha256':record['sha256']})
  items.append(record)
record_by_path={x['path']:x for x in items}
for r in items:
 for parent in pathlib.PurePosixPath(r['path']).parents:
  if str(parent) in record_by_path and record_by_path[str(parent)]['type']!='directory':
   r['errors'].append('file ancestor collision'); problems.append(r)
summary={'archive':str(archive),'archive_sha256':expected,'members':len(items),'regular_files':sum(x['type']=='file' for x in items),'directories':sum(x['type']=='directory' for x in items),'links':sum(x['type'] in ('symlink','hardlink') for x in items),'payload_bytes':total,'casefold_unique_paths':len(seen),'problems':problems,'selected_text_files':selected,'safe_for_regular_file_extraction':not problems}
(out/'tar-members.json').write_text(json.dumps(items,indent=2),encoding='utf-8')
(out/'tar-audit.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in summary.items() if k!='selected_text_files'},indent=2))
print('Selected licenses: '+json.dumps([x['path'] for x in selected if x['is_license']]))
print('CMake files: '+str(sum(x['is_cmake'] for x in selected)))
if problems: raise SystemExit('Archive rejected; inspect problems before extraction')

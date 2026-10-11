import hashlib,json,pathlib,tarfile,struct,re
base=pathlib.Path('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages')
audit=base/'libsamplerate-rev2-package-audit'
archive=base/'libsamplerate-rev2-build/packages/libsamplerate-0.2.1-je-rev2-windows.tar.xz'
payload=base/'libsamplerate-rev2-build/packages/libsamplerate-0.2.1-je-rev2-windows.contents'
dest=audit/'extracted/libsamplerate-0.2.1-je-rev2-windows'
source=pathlib.Path('F:/JericoEngine/Dependencies/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b')
provider=pathlib.Path('F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/LibSamplerate')
engine=pathlib.Path('F:/JericoEngine/JericoEngine')
pin='01086CD19A1D241B4B0504FE16142C80BF29AF329543E67FF773301EDAA95122'
sha=lambda b:hashlib.sha256(b).hexdigest().upper()
def save(name,obj):
 with (audit/name).open('x',encoding='utf-8',newline='\n') as f:json.dump(obj,f,indent=2);f.write('\n')
def no_reparse(path):
 for p in (path,)+tuple(path.parents):
  if p.exists() and p.lstat().st_file_attributes & 1024:raise RuntimeError('Reparse path rejected')
no_reparse(dest)
assert not dest.exists()
raw=archive.read_bytes();assert sha(raw)==pin and len(raw)==3954496
files={};seen=set()
with tarfile.open(archive,'r:xz') as tar:
 for m in tar.getmembers():
  name=m.name;parts=name.split('/')
  assert m.isreg() and not m.linkname and not name.startswith('/') and '\\' not in name and ':' not in name
  for p in parts:
   assert p and p not in ('.','..') and not p.endswith(('.',' ')) and not any(c in p for c in '<>:"|?*')
   assert not re.match(r'^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)',p,re.I)
  assert name.casefold() not in seen;seen.add(name.casefold())
  assert 0<=m.size<=50000000
  with tar.extractfile(m) as s:b=s.read()
  assert len(b)==m.size;files[name]=b
for name in files:
 assert not any(str(p).casefold() in seen for p in pathlib.PurePosixPath(name).parents)
expected={'Findlibsamplerate.cmake','JERequireLocalPath.cmake','NOTICE.txt','README.md','PackageInfo.json','SHA256SUMS','libsamplerate/JERecipe.json','libsamplerate/COPYING','libsamplerate/include/samplerate.h','libsamplerate/lib/samplerate.lib','LICENSES/libsamplerate/COPYING','LICENSES/libsamplerate/SourceManifest.json','LICENSES/O3DE/LICENSE.txt','LICENSES/O3DE/LICENSE_APACHE2.TXT','LICENSES/O3DE/LICENSE_MIT.TXT'}
for p in sorted((source/'src').iterdir()):
 if p.suffix in ('.c','.h'):expected.add('LICENSES/libsamplerate/source-notices/'+p.name)
expected.add('LICENSES/libsamplerate/source-notices/samplerate.h')
expected.update({'LICENSES/libsamplerate/patch-provenance/samplerate.c.original','LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.patch','LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.json'})
assert len(files)==27 and set(files)==expected,(set(files)-expected,expected-set(files))
sums={}
for line in files['SHA256SUMS'].decode().splitlines():
 digest,name=line.split(' *',1);assert re.fullmatch('[0-9a-f]{64}',digest) and name not in sums;sums[name]=digest.upper()
assert len(sums)==26 and set(sums)==set(files)-{'SHA256SUMS'}
records=[]
for name,b in sorted(files.items()):
 digest=sha(b);assert name=='SHA256SUMS' or digest==sums[name]
 no_reparse(payload/name);assert b==(payload/name).read_bytes()
 records.append({'path':name,'type':'regular','bytes':len(b),'sha256':digest})
info=json.loads(files['PackageInfo.json'])
assert info['name']=='libsamplerate-0.2.1-je-rev2-windows' and info['version']=='0.2.1' and info['license']=='BSD-2-Clause'
assert info['source']=={'commit':'a147b6dc4046deda22b2aa772b348717de772e2b','sha256':'f90e46bbf9bae05139e297ceebd5394ef77ce5e294e6b46beffcc15b9446a041','url':'https://codeload.github.com/libsndfile/libsamplerate/tar.gz/a147b6dc4046deda22b2aa772b348717de772e2b'}
pairs=[('libsamplerate/COPYING',source/'COPYING'),('LICENSES/libsamplerate/COPYING',source/'COPYING'),('libsamplerate/include/samplerate.h',source/'include/samplerate.h'),('LICENSES/libsamplerate/source-notices/samplerate.h',source/'include/samplerate.h'),('LICENSES/libsamplerate/SourceManifest.json',base/'libsamplerate-rev1-audit/source-manifest.json'),('JERequireLocalPath.cmake',engine/'cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake')]
for name in ('Findlibsamplerate.cmake','README.md','NOTICE.txt'):pairs.append((name,provider/name))
for name in ('LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT'):pairs.append(('LICENSES/O3DE/'+name,engine/name))
for p in sorted((source/'src').iterdir()):
 if p.suffix in ('.c','.h'):pairs.append(('LICENSES/libsamplerate/source-notices/'+p.name,p))
# The sole modified notice is checked against the exact private compiled source,
# independently reconstructed from the immutable original as bytes, not executed.
original=(source/'src/samplerate.c').read_bytes()
metadata=json.loads(files['LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.json'])
assert sha(original)==metadata['originalSHA256'].upper() and len(original)==metadata['originalBytes']==12951
applicator=(provider/'ApplySamplerateChannelGuard.cmake').read_text(encoding='utf-8')
before=re.search(r'set\(JE_PATCH_BEFORE \[=\[(.*?)\]=\]\)',applicator,re.S).group(1).encode()
after=re.search(r'set\(JE_PATCH_AFTER \[=\[(.*?)\]=\]\)',applicator,re.S).group(1).encode()
assert original.count(before)==1
modified=original.replace(before,after)
assert sha(modified)==metadata['modifiedSHA256'].upper() and len(modified)==metadata['modifiedBytes']==13047
assert metadata['sourceCommit']=='a147b6dc4046deda22b2aa772b348717de772e2b' and metadata['path']=='src/samplerate.c' and metadata['hunks']==1
assert metadata['modifiedSHA256']=='14238804216c63fc3cd5b42cdb2fde1086cc510cac2f6c13da4bafab55076fae'
modified_path=base/'libsamplerate-rev2-build/source-work/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b/src/samplerate.c'
pairs=[(name,modified_path if name=='LICENSES/libsamplerate/source-notices/samplerate.c' else path) for name,path in pairs]
pairs.extend([('LICENSES/libsamplerate/patch-provenance/samplerate.c.original',source/'src/samplerate.c'),('LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.patch',provider/'SamplerateChannelGuard.patch'),('LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.json',provider/'SamplerateChannelGuard.json')])
assert files['LICENSES/libsamplerate/source-notices/samplerate.c']==modified
for name,p in pairs:no_reparse(p);assert files[name]==p.read_bytes(),('notice/API/Find mismatch',name)
recipe=json.loads(files['libsamplerate/JERecipe.json'])
assert recipe['version']=='0.2.1' and recipe['CRT']=='/MD' and recipe['language']=='C99'
assert recipe['sourcePatches']==['LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.json']
assert all(recipe[key] is True for key in ('SINC_FAST','SINC_MEDIUM','SINC_BEST'))
assert all(recipe[key] is False for key in ('BUILD_SHARED_LIBS','BUILD_TESTING','LIBSAMPLERATE_EXAMPLES','LIBSAMPLERATE_INSTALL'))
save('patch-provenance-audit.json',{'status':'STATIC_BYTE_AUDIT_ONLY','immutableOriginalSHA256':sha(original),'originalBytes':len(original),'modifiedNoticeSHA256':sha(modified),'modifiedBytes':len(modified),'originalNoticeByteExact':True,'modifiedNoticeMatchesCompiledWorkcopyAndStrictPatch':True,'metadataAndPatchMatchFrozenProvider':True,'originalSourceWritten':False,'sourcePatches':recipe['sourcePatches'],'metadata':metadata,'nativeExecuted':False})
# MSVC static COFF archive only: no dynamic import objects; inspect object
# headers, symbols and CRT directives as bytes without loading code/tools.
lib=files['libsamplerate/lib/samplerate.lib'];assert lib.startswith(b'!<arch>\n')
offset=8;members=[];definitions=set();undefined=set();directives=[];longnames=b''
while offset<len(lib):
 assert offset+60<=len(lib)
 header=lib[offset:offset+60];assert header[58:60]==b'`\n'
 name=header[:16].decode('ascii').strip();size=int(header[48:58]);start=offset+60;end=start+size;assert end<=len(lib);data=lib[start:end]
 if name=='//':longnames=data
 elif name not in ('/','/SYM64/'):
  display=name
  if name.startswith('/'):
   i=int(name[1:]);assert i<len(longnames);display=longnames[i:].split(b'\0',1)[0].decode('ascii')
  assert len(data)>=20
  machine,sections,stamp,sympos,symcount,optional,flags=struct.unpack_from('<HHIIIHH',data)
  assert machine==0x8664 and optional==0 and 20+sections*40<=len(data),('non-AMD64 object/import stub',display)
  assert sympos+symcount*18+4<=len(data)
  stringpos=sympos+symcount*18;stringlen=struct.unpack_from('<I',data,stringpos)[0];assert stringlen>=4 and stringpos+stringlen<=len(data)
  strings=data[stringpos:stringpos+stringlen]
  for i in range(sections):
   sh=data[20+i*40:60+i*40];secname=sh[:8].rstrip(b'\0').decode('ascii');length,ptr=struct.unpack_from('<II',sh,16)
   if length:assert ptr+length<=len(data)
   if secname=='.drectve':directives.append({'member':display,'text':data[ptr:ptr+length].decode('ascii').strip('\0')})
  i=0;defined=0
  while i<symcount:
   symbol=data[sympos+i*18:sympos+(i+1)*18];zero,idx=struct.unpack_from('<II',symbol)
   if zero==0:
    assert 4<=idx<stringlen;sname=strings[idx:].split(b'\0',1)[0].decode('ascii')
   else:sname=symbol[:8].split(b'\0',1)[0].decode('ascii')
   value,section,stype,storage,aux=struct.unpack_from('<IhHBB',symbol,8)
   if storage==2:
    if section>0:definitions.add(sname);defined+=1
    elif section==0:undefined.add(sname)
   assert i+aux<symcount;i+=1+aux
  members.append({'member':display,'machine':'AMD64','bytes':len(data),'symbolCount':symcount,'externalDefinitions':defined})
 offset=end+(size%2)
assert offset==len(lib) and len(members)==4
header=re.sub(rb'/\*.*?\*/',b'',files['libsamplerate/include/samplerate.h'],flags=re.S)
api=sorted(set(n.decode() for n in re.findall(rb'\b(src_[A-Za-z0-9_]+)\s*\(',header)))
assert len(api)==20 and set(api)<=definitions,(api,set(api)-definitions)
assert not any(re.search(r'(?:^_?Py_|^_?PyInit_|python|pybind)',n,re.I) for n in definitions|undefined)
assert b'libsamplerate-0.2.1 (c) 2002-2008 Erik de Castro Lopo' in lib
assert any('MSVCRT' in d['text'] for d in directives)
assert not any('MSVCRTD' in d['text'] for d in directives)
save('coff-static-library-audit.json',{'status':'STATIC_DATA_AUDIT_ONLY','libraryBytes':len(lib),'librarySHA256':sha(lib),'machine':'AMD64','objectMembers':members,'apiDefinitions':api,'apiDefinitionsCount':len(api),'dynamicImportObjects':0,'pythonSymbolReferences':0,'directives':directives,'loadedOrExecuted':False,'CRTLimit':'Release MSVCRT directives observed; no general mixed-CRT or legacy binary compatibility claim'})
save('archive-audit-before-extraction.json',{'archive':str(archive),'archiveBytes':len(raw),'archiveSHA256':pin,'regularFiles':len(records),'links':0,'caseCollisions':0,'contentManifestEntries':len(sums),'manifestCoverageComplete':True,'payloadByteExact':True,'sourceNoticeFindPairs':len(pairs),'sourceNoticeFindPairsExact':True,'packageInfo':info,'entries':records,'nativeExecuted':False})
dest.mkdir(parents=True)
for name,b in files.items():
 p=dest/name;p.parent.mkdir(parents=True,exist_ok=True)
 with p.open('xb') as f:f.write(b)
 assert p.read_bytes()==b
save('extracted-package-manifest.json',{'root':str(dest),'count':len(records),'entries':records,'byteExactArchive':True})
save('package-audit-summary.json',{'status':'ARCHIVE_STATIC_COFF_AND_FRESH_EXTRACTION_VERIFIED_DATA_ONLY','archiveBytes':len(raw),'archiveSHA256':pin,'files':len(records),'contentManifestEntries':len(sums),'sourceNoticeFindPairs':len(pairs),'staticObjectCount':len(members),'apiDefinitions':len(api),'root':str(dest),'nativeConsumerNotRun':True,'guardsNotRun':True,'engineAccepted':False})
print(json.dumps({'archiveBytes':len(raw),'files':len(records),'sums':len(sums),'pairs':len(pairs),'staticObjects':len(members),'API':len(api),'status':'DATA_AUDIT_ONLY'}))
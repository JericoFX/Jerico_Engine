import tarfile, hashlib, json, pathlib, re, base64
root=pathlib.Path(__file__).parent
archive=pathlib.Path('F:/JericoEngine/Dependencies/archives/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b.tar.gz')
snapshot=pathlib.Path('F:/JericoEngine/Dependencies/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b')
commit='a147b6dc4046deda22b2aa772b348717de772e2b'
expected='f90e46bbf9bae05139e297ceebd5394ef77ce5e294e6b46beffcc15b9446a041'
def sha(b):return hashlib.sha256(b).hexdigest().upper()
def save(name,obj):
 with (root/name).open('x',encoding='utf-8',newline='\n') as f:json.dump(obj,f,indent=2);f.write('\n')
assert sha(archive.read_bytes()).lower()==expected
assert not snapshot.exists()
tree=json.loads((root/'primary-tree.json').read_text())
assert not tree.get('truncated')
blobs={x['path']:x for x in tree['tree'] if x['type']=='blob'}
assert all(x['type'] in ('tree','blob') and x['mode'] not in ('120000','160000') for x in tree['tree'])
entries=[]; files={};seen={}
with tarfile.open(archive,'r:gz') as tar:
 for m in tar.getmembers():
  assert m.isfile() or m.isdir(),('unsupported entry',m.name,m.type)
  name=m.name.rstrip('/')
  assert '\\' not in name and ':' not in name and not name.startswith('/')
  parts=name.split('/');assert parts[0]=='libsamplerate-'+commit
  assert all(p and p not in ('.','..') and not p.endswith((' ','.')) and not re.match(r'^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)',p,re.I) for p in parts)
  key=name.casefold();assert key not in seen;seen[key]=name
  rel='/'.join(parts[1:])
  entries.append({'archivePath':name,'path':rel,'type':'file' if m.isfile() else 'directory','bytes':m.size})
  if m.isfile():
   b=tar.extractfile(m).read();assert len(b)==m.size
   assert rel in blobs,('missing Git tree path',rel)
   blob=hashlib.sha1(('blob '+str(len(b))+'\0').encode()+b).hexdigest()
   if blob!=blobs[rel]['sha']:
    assert rel=='Win32/libsamplerate-0.def'
    raw=(root/'primary-def.raw').read_bytes()
    api=json.loads((root/'primary-def-blob.json').read_text())
    primary=base64.b64decode(api['content']);assert primary==raw
    assert api['sha']==blobs[rel]['sha'] and len(primary)==blobs[rel]['size']
    assert hashlib.sha1(('blob '+str(len(primary))+'\0').encode()+primary).hexdigest()==blobs[rel]['sha']
    assert b.replace(b'\r\n',b'\n')==primary and b.count(b'\r\n')==30
    assert b'*.def text eol=crlf' in files['.gitattributes']
    save('source-eol-attribution-proof.json',{'path':rel,'attributePath':'.gitattributes','attributeGitBlobSHA1':blobs['.gitattributes']['sha'],'attributeRule':'*.def text eol=crlf','rawArchiveBytes':len(b),'rawArchiveSHA256':sha(b),'rawGitBlobSHA1':blob,'primaryBytes':len(primary),'primarySHA256':sha(primary),'primaryGitBlobSHA1':api['sha'],'primaryRawEqualsAPIBlob':True,'difference':'Exactly30 CRLF versus LF pairs; every other byte equal; comparison only, no rewrite','rawExactOtherFiles':98,'archiveSourceRewritten':False})
   else:assert len(b)==blobs[rel]['size']
   files[rel]=b;entries[-1].update(sha256=sha(b),gitBlobSHA1=blob)
assert set(files)==set(blobs),(set(blobs)-set(files),set(files)-set(blobs))
for path in files:
 assert not any('/'.join(path.split('/')[:i]) in files for i in range(1,len(path.split('/'))))
manifest=[{'path':rel,'bytes':len(b),'sha256':sha(b),'gitBlobSHA1':blobs[rel]['sha']} for rel,b in sorted(files.items())]
save('archive-audit-before-extraction.json',{'status':'SAFE_ARCHIVE_AND_RAW_PRIMARY_BLOBS_VERIFIED','archive':str(archive),'archiveBytes':archive.stat().st_size,'archiveSHA256':sha(archive.read_bytes()),'commit':commit,'regularFiles':len(files),'entries':entries,'links':0,'pathCaseCollisions':0,'rawExactGitBlobs':98,'attributeExplainedCRLFFiles':1,'sourceExecuted':False})
snapshot.mkdir()
for rel,b in sorted(files.items()):
 dst=snapshot/rel;dst.parent.mkdir(parents=True,exist_ok=True)
 with dst.open('xb') as f:f.write(b)
 assert sha(dst.read_bytes())==sha(b)
save('source-manifest.json',{'sourceRoot':str(snapshot),'count':len(manifest),'entries':manifest})
save('source-extraction-receipt.json',{'status':'EXTRACTED_NEW_SNAPSHOT_RAW_EXACT','sourceRoot':str(snapshot),'files':len(manifest),'rawExactGitBlobs':98,'attributeExplainedCRLFFiles':1,'archiveSHA256':expected.upper(),'sourceExecuted':False,'manifestSHA256':sha((root/'source-manifest.json').read_bytes())})
import tarfile,hashlib,json,pathlib
root=pathlib.Path(__file__).parent
archive=pathlib.Path('F:/JericoEngine/Dependencies/archives/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b.tar.gz')
tree=json.loads((root/'primary-tree.json').read_text());blobs={x['path']:x for x in tree['tree'] if x['type']=='blob'}
def blob(b):return hashlib.sha1(('blob '+str(len(b))+'\0').encode()+b).hexdigest()
changes=[];attrs=[];exact=0
with tarfile.open(archive,'r:gz') as tar:
 for m in tar.getmembers():
  if not m.isfile():continue
  rel='/'.join(m.name.split('/')[1:]);b=tar.extractfile(m).read();actual=blob(b)
  if rel.endswith('.gitattributes'):attrs.append({'path':rel,'text':b.decode(),'rawGitBlobExact':actual==blobs[rel]['sha']})
  if actual==blobs[rel]['sha']:exact+=1
  else:changes.append({'path':rel,'rawBytes':len(b),'rawSHA256':hashlib.sha256(b).hexdigest(),'rawGitBlobSHA1':actual,'primaryGitBlobSHA1':blobs[rel]['sha'],'crlfCount':b.count(b'\r\n'),'lfCount':b.count(b'\n'),'normalizedGitBlobSHA1':blob(b.replace(b'\r\n',b'\n')),'normalizedMatchesGit':blob(b.replace(b'\r\n',b'\n'))==blobs[rel]['sha']})
with (root/'source-byte-comparison-red.json').open('x',encoding='utf-8') as f:json.dump({'rawExact':exact,'differences':changes,'attributes':attrs,'extracted':False},f,indent=2)
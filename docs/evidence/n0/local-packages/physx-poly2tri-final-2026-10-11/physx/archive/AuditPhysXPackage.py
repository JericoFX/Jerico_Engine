import hashlib,json,pathlib,re,tarfile
root=pathlib.Path("F:/JericoEngine/JericoEngine")
base=root/"build/n0-validation/local-packages"
audit=base/"root-physx-poly2tri-rev1/physx-package-audit"
archive=base/"physx-local-rev2/packages/PhysX-5.1.1-je-cpu-rev2-windows.tar.xz"
payload=archive.parent/"PhysX-5.1.1-je-cpu-rev2-windows.contents"
source=pathlib.Path("F:/JericoEngine/Dependencies/PhysX-0bbcff3d0c541325f4d14c36ee18f24e22e35e6e")
provider=root/"cmake/Tools/LocalPackages/PhysX"
witness_path=base/"physx-stage-preparation-rev1/native-artifact-witness.json"
source_manifest_path=base/"physx-rev1-acquisition/source-snapshot-manifest.json"
archive_pin="6A3655FA52AD06691F1B360B650557518C06494AC0E4EA7418158B7B63F4FC62"
witness_pin="D1E1704717F36BA1602461B434F152FD161AA446ABBD7E62CFD85C5F0288D693"
source_manifest_pin="202329A844DF20F8E3349C589A3D9567F7D633615B2B17E37FD49929F03AC2AD"
def no_reparse(path):
 for p in (path,)+tuple(path.parents):
  if p.exists() and p.lstat().st_file_attributes & 1024:raise RuntimeError("Reparse input rejected: "+str(p))
def digest(path):
 no_reparse(path)
 h=hashlib.sha256()
 with path.open("rb") as f:
  for b in iter(lambda:f.read(1024*1024),b""):h.update(b)
 return h.hexdigest().upper()
def save(name,data):
 with (audit/name).open("x",encoding="utf-8",newline="\n") as f:json.dump(data,f,indent=2);f.write("\n")
assert digest(archive)==archive_pin and archive.stat().st_size==73160792
assert digest(witness_path)==witness_pin and digest(source_manifest_path)==source_manifest_pin
witness=json.loads(witness_path.read_bytes());manifest=json.loads(source_manifest_path.read_bytes())
assert len(witness["libraries"])==56 and len(manifest["entries"])==1788
expected={};pairs={}
def add(name,path,sha=None,size=None):
 assert name not in expected
 no_reparse(path)
 expected[name]={"sha256":sha or digest(path),"bytes":path.stat().st_size if size is None else size}
 pairs[name]=path
for row in manifest["entries"]:
 if row["path"].startswith("physx/include/") or row["path"]=="physx/version.txt":
  add("PhysX/"+row["path"],source/row["path"],row["sha256"].upper(),row["bytes"])
header=witness["generatedHeader"];header_name="PhysX/physx/include/PxConfig.h"
expected.pop(header_name,None);pairs.pop(header_name,None)
add(header_name,pathlib.Path(header["path"]),header["sha256"].upper())
main={"PhysX","PhysXPvdSDK","PhysXVehicle","PhysXCharacterKinematic","PhysXExtensions","PhysXCooking","PhysXCommon","PhysXFoundation"}
configs={"debug","checked","profile","release"}
libs=[];seen_pairs=set()
for row in witness["libraries"]:
 if row["target"] not in main:continue
 assert row["config"] in configs and (row["config"],row["target"]) not in seen_pairs
 seen_pairs.add((row["config"],row["target"]))
 name="PhysX/physx/bin/static/"+row["config"]+"/"+row["target"]+"_static_64.lib"
 add(name,pathlib.Path(row["path"]),row["sha256"].upper(),row["bytes"]);libs.append(name)
assert len(libs)==32 and len(seen_pairs)==32
add("PhysX/JEStaticArtifactWitness.json",witness_path,witness_pin)
for name in ("LICENSE.md","README.md"):add("LICENSES/PhysX/"+name,source/name)
for name in ("FindPhysX5.cmake","NOTICE.txt","Recipe.json"):add(name,provider/name)
add("JERequireLocalPath.cmake",root/"cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake")
recipe=json.loads((provider/"Recipe.json").read_bytes())
assert recipe["identity"]=="PhysX-5.1.1-je-cpu-rev2-windows" and recipe["stagedStaticLibraries"]==32
for row in recipe["helperOriginals"]:add("LICENSES/official-build-helpers/"+row["path"],provider/"official-build-helpers"/row["path"],row["sha256"])
assert len(recipe["helperOriginals"])==6
for name in ("LICENSE.txt","LICENSE_APACHE2.TXT","LICENSE_MIT.TXT"):add("LICENSES/O3DE/"+name,root/name)
expected_names=set(expected)|{"PackageInfo.json","SHA256SUMS"}
rows=[];small={};total=0
with tarfile.open(archive,"r:xz") as tar:
 members=tar.getmembers();seen=set()
 for m in members:
  name=m.name;parts=name.split("/")
  assert m.isreg() and not m.linkname and not name.startswith("/") and "\\" not in name and ":" not in name
  for part in parts:
   assert part and part not in (".","..") and not part.endswith(("."," ")) and not any(c in part for c in '<>:"|?*')
   assert not re.match(r"^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)",part,re.I)
  assert name.casefold() not in seen;seen.add(name.casefold())
  assert 0<=m.size<=100*1024*1024
  total+=m.size;assert total<2*1024*1024*1024
 assert {m.name for m in members}==expected_names,("Unexpected/missing members",sorted({m.name for m in members}-expected_names),sorted(expected_names-{m.name for m in members}))
 for name in expected_names:assert not any(str(p).casefold() in seen for p in pathlib.PurePosixPath(name).parents)
 # Stream all libraries; compare actual bytes with both native outputs and payload.
 for m in members:
  h=hashlib.sha256();count=0;cached=bytearray()
  no_reparse(payload/m.name)
  with tar.extractfile(m) as data,(payload/m.name).open("rb") as copy:
   reference=pairs[m.name].open("rb") if m.name in pairs else None
   try:
    for b in iter(lambda:data.read(1024*1024),b""):
     count+=len(b);h.update(b);assert copy.read(len(b))==b,("Payload byte mismatch",m.name)
     if reference:assert reference.read(len(b))==b,("Source/native byte mismatch",m.name)
     if m.name in ("PackageInfo.json","SHA256SUMS","Recipe.json","NOTICE.txt","LICENSES/PhysX/LICENSE.md"):cached.extend(b)
    assert not copy.read(1)
    if reference:assert not reference.read(1)
   finally:
    if reference:reference.close()
  sha=h.hexdigest().upper();assert count==m.size
  if m.name in expected:assert sha==expected[m.name]["sha256"].upper() and count==expected[m.name]["bytes"],("Input witness mismatch",m.name)
  rows.append({"path":m.name,"type":"regular","bytes":count,"sha256":sha})
  if cached:small[m.name]=bytes(cached)
sums={}
for line in small["SHA256SUMS"].decode("utf-8").splitlines():
 sha,name=line.split(" *",1);assert re.fullmatch("[0-9a-f]{64}",sha) and name not in sums;sums[name]=sha.upper()
assert set(sums)==expected_names-{"SHA256SUMS"}
for row in rows:
 if row["path"]!="SHA256SUMS":assert row["sha256"]==sums[row["path"]]
info=json.loads(small["PackageInfo.json"])
assert info["name"]=="PhysX-5.1.1-je-cpu-rev2-windows" and info["version"]=="5.1.1" and info["license"]=="BSD-3-Clause; original notices preserved"
assert info["source"]=={"commit":"0bbcff3d0c541325f4d14c36ee18f24e22e35e6e","sha256":"a2dfa049b0c7fa67723d27d70520d0ed7bcb5dd3c2d9df8dfb169d8a39c4c142","url":"https://codeload.github.com/NVIDIA-Omniverse/PhysX/tar.gz/0bbcff3d0c541325f4d14c36ee18f24e22e35e6e"}
assert b"Redistribution and use" in small["LICENSES/PhysX/LICENSE.md"]
assert digest(archive)==archive_pin
save("archive-member-manifest.json",{"status":"DATA_AUDIT_ONLY_PARENT_ACCEPTANCE_PENDING","archive":str(archive),"archiveSHA256":archive_pin,"archiveBytes":73160792,"memberCount":len(rows),"entries":sorted(rows,key=lambda r:r["path"])})
save("package-audit-summary.json",{"status":"DATA_AUDIT_COMPLETED_NO_PACKAGE_CODE_EXECUTED","archiveSHA256":archive_pin,"archiveBytes":73160792,"members":len(rows),"internalHashes":len(sums),"totalUncompressedBytes":total,"safeRegularMembersOnly":True,"noLinksTraversalCaseOrPrefixCollisions":True,"exactExpectedMembership":True,"sourceNoticeHeaderAndHelperPairsByteExact":len(pairs),"mainLibrariesByteExactAgainstNativeWitnessAndPayload":len(libs),"libraryConfigs":sorted(configs),"libraryTargets":sorted(main),"auxiliaryPayloadLibraries":0,"originalWitnessLibraries":56,"witnessSHA256":witness_pin,"sourceManifestSHA256":source_manifest_pin,"packageInfo":info,"extractionPerformed":False,"limitations":["Data and exact SDK source/native-output comparison only","No new PE/COFF analysis or execution; existing parent-reviewed narrow link audit reused","No production target/Editor/Jolt/runtime or historical ABI acceptance"]})
print(json.dumps({"status":"DATA_AUDIT_COMPLETED","members":len(rows),"hashes":len(sums),"libraries":len(libs),"archiveSHA256":archive_pin}))

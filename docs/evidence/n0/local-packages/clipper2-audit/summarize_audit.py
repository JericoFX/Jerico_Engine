import hashlib,json,pathlib,re
out=pathlib.Path(r'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\clipper2-audit')
root=pathlib.Path(r'F:\JericoEngine\Dependencies\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4')
manifold=pathlib.Path(r'F:\JericoEngine\Dependencies\manifold-3.5.1')
repo=pathlib.Path(r'F:\JericoEngine\JericoEngine')
manifest=json.loads((out/'source-manifest.json').read_text())
for item in manifest:
 p=root/pathlib.Path(*pathlib.PurePosixPath(item['path']).parts)
 if hashlib.sha256(p.read_bytes()).hexdigest()!=item['sha256']: raise SystemExit('Clipper source changed '+item['path'])
before=json.loads((out/'manifold-source-before.json').read_text())
manifold_checks=[]
for item in before:
 p=manifold/pathlib.Path(*pathlib.PurePosixPath(item['path']).parts)
 sha=hashlib.sha256(p.read_bytes()).hexdigest()
 manifold_checks.append({'path':item['path'],'sha256':sha,'match':sha.lower()==item['sha256'].lower()})
if any(not x['match'] for x in manifold_checks) or len([p for p in manifold.rglob('*') if p.is_file()])!=len(before): raise SystemExit('Immutable Manifold changed')
(out/'manifold-source-after.json').write_text(json.dumps({'files':len(before),'all_match':True,'checks':manifold_checks},indent=2),encoding='utf-8')
patterns={'fetchcontent':r'\bFetchContent(?:_\w+)?\b','externalproject':r'\bExternalProject(?:_\w+)?\b','file_download':r'\bfile\s*\(\s*DOWNLOAD\b','execute_process':r'\bexecute_process\s*\(','git_acquisition':r'\b(?:git\s+(?:clone|submodule)|"git"\s+"clone")\b','submodule_reference':r'\bsubmodule\b'}
files=[]; hits={k:[] for k in patterns}
for p in root.rglob('*'):
 if not p.is_file() or not (p.name.lower().startswith('cmakelists.txt') or '.cmake' in p.name.lower()): continue
 text=p.read_text(encoding='utf-8',errors='replace'); rel=str(p.relative_to(root)).replace('\\','/')
 files.append({'path':rel,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size})
 for key,pattern in patterns.items():
  for match in re.finditer(pattern,text,re.I):
   line=text.count('\n',0,match.start())+1
   hits[key].append({'path':rel,'line':line,'text':text.splitlines()[line-1].strip()})
closure={'scope':'static audit only; no upstream CMake/Git/patch execution','files':files,'hits':hits,'conditional_network':['CPP/CMakeLists.txt:225 CLIPPER2_TESTS ON may execute Git clone googletest without a pinned revision','CPP/CMakeLists.txt:313 USE_EXTERNAL_GBENCHMARK ON adds BenchMark whose CMake fetches Google benchmark v1.7.1'],'network_with_recommended_options':[],'source_write':'CPP/CMakeLists.txt:38-40 configure_file writes CPP/Clipper2Lib/include/clipper2/clipper.version.h in source; must use work copy even though generated header currently matches','manifold_default':'MANIFOLD_CROSS_SECTION ON, backend clipper2; no feature removed','manifold_nested_declaration':'real WhiteBox wrapper declares Clipper2 first via o3de_fetch_content SOURCE_SUBDIR CPP, before manifoldDeps declaration; installed FetchContent first declaration wins','manifold_carry_patch':'manifoldDeps declares a Clipper2 PATCH_COMMAND using git apply for NO_IOSTREAM; standard source override skips population/patch, first wrapper declaration also has no PATCH_COMMAND. Do not claim patch applied. Current MANIFOLD_NO_IOSTREAM OFF needs no such change; do not enable without separate audited repair/validation','offline_guard_case':'manifoldDeps logmissingdep(PKG) checks DEFINED FETCHCONTENT_SOURCE_DIR_${PKG}; called with Clipper2, not uppercase CLIPPER2. With MANIFOLD_DOWNLOADS OFF and no Clipper2_FOUND, supply exact mixed-case alias to the same work path alongside standard uppercase override, or parent must separately fix/validate upstream guard. Candidate argv not executed.'}
(out/'cmake-closure-audit.json').write_text(json.dumps(closure,indent=2),encoding='utf-8')
licenses=[]
for rel in ('LICENSE','CSharp/NuGet/License.txt','CSharp/Clipper2Lib/HashCode.cs'):
 p=root/rel; text=p.read_text(encoding='utf-8')
 if rel.endswith('HashCode.cs'): text=text[text.index('  /*'):text.index('  */')+4]
 licenses.append({'path':rel,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'notice_text':text})
(out/'license-notices.json').write_text(json.dumps({'cpp_library':'BSL-1.0; Angus Johnson copyright notices preserved in each source/header','archive_other_notice':'CSharp/Clipper2Lib/HashCode.cs declares .NET Foundation MIT plus embedded Yann Collet xxHash BSD-2-Clause; not consumed by CPP CMake and not relabeled BSL','redistribution':'No legal deployment acceptance; preserve notices and review complete MIT text if CSharp is ever redistributed','notices':licenses},indent=2),encoding='utf-8')
work_clip=r'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root'
work_man=r'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/manifold-3.5.1-root'
argv=[f'-DFETCHCONTENT_SOURCE_DIR_MANIFOLD:PATH={work_man}',f'-DFETCHCONTENT_SOURCE_DIR_CLIPPER2:PATH={work_clip}',f'-DFETCHCONTENT_SOURCE_DIR_Clipper2:PATH={work_clip}','-DMANIFOLD_CROSS_SECTION:BOOL=ON','-DMANIFOLD_CROSS_SECTION_BACKEND:STRING=clipper2','-DMANIFOLD_USE_BUILTIN_CLIPPER2:BOOL=ON','-DMANIFOLD_DOWNLOADS:BOOL=OFF','-DMANIFOLD_PAR:BOOL=OFF','-DMANIFOLD_TEST:BOOL=OFF','-DMANIFOLD_CBIND:BOOL=OFF','-DMANIFOLD_PYBIND:BOOL=OFF','-DTRACY_ENABLE:BOOL=OFF','-DMANIFOLD_FUZZ:BOOL=OFF','-DASSIMP_ENABLE:BOOL=OFF','-DMANIFOLD_NO_IOSTREAM:BOOL=OFF','-DCLIPPER2_UTILS:BOOL=OFF','-DCLIPPER2_EXAMPLES:BOOL=OFF','-DCLIPPER2_TESTS:BOOL=OFF','-DUSE_EXTERNAL_GBENCHMARK:BOOL=OFF','-DCLIPPER2_USINGZ:STRING=OFF']
(out/'root-input-candidate.json').write_text(json.dumps({'executed':False,'work_copies_created':False,'paths':'proposed paths only; parent must create source copies and freeze their initial manifests','arguments':argv,'existing_wrapper_forces':['MANIFOLD_PAR/TEST/CBIND/PYBIND/DEBUG OFF and scoped BUILD_SHARED_LIBS OFF'],'existing_protections':'preserve no-server, validated local package/cache, FETCHCONTENT_FULLY_DISCONNECTED ON; do not replace with unprotected command','numeric_options':'leave CLIPPER2_HI_PRECISION OFF and CLIPPER2_MAX_DECIMAL_PRECISION 8 defaults unchanged','additional_case_alias':'exact mixed-case variable only satisfies upstream logmissingdep guard; uppercase remains actual provider/source override','cpp_source_subdir':'CPP supplied by wrapper declaration; source path must be Clipper root, not /CPP'},indent=2),encoding='utf-8')
header=(root/'CPP/Clipper2Lib/include/clipper2/clipper.version.h').read_bytes()
expected=(root/'CPP/clipper.version.in').read_bytes().replace(b'@PROJECT_VERSION@',b'2.0.1').replace(b'\r\n',b'\n')
summary={'scope':'manual audited acquisition/static source closure only; no source/config/docs/Git/CMake/build/test edits/execution','url':'https://codeload.github.com/AngusJohnson/Clipper2/tar.gz/46f639177fe418f9689e8ddb74f08a870c71f5b4','head_get_status':200,'redirects':0,'credentials':'none','archive_bytes':716581,'archive_sha256':'73f5783e0c88299976334f48e3e356c756b96212c652ecc8bceec3bd99455bcb','commit':'46f639177fe418f9689e8ddb74f08a870c71f5b4','CPP_project_version':'2.0.1','source_root':str(root),'members':284,'files':223,'directories':61,'payload_bytes':3006633,'links':0,'reparse_points':0,'collisions':0,'all_clipper_hashes_match':True,'all_manifold_hashes_match':True,'manifold_files_unchanged':len(before),'gitmodules':'present but zero bytes; no submodule/Git checkout entries','license':'CPP BSL-1.0; complete archive also preserves CSharp MIT/BSD2 notices outside build closure','cpp_cmake_files':len(files),'source_side_effect':'Clipper configure_file generates header in source, parent must copy under build; immutable snapshot not used directly','generated_header_currently_matches_original_bytes':header==expected,'Manifold_source_side_effect':'real wrapper patches csg_tree.h and edge_op.cpp, parent must use work copy; immutable source unchanged','source_subdir':'CPP','candidate_argv':'root-input-candidate.json; not executed, no work copies created','offline_caveat':'mixed-case source guard alias required for MANIFOLD_DOWNLOADS OFF with current source; validate actual root CMake/cache/target closure with Luna','product_acceptance':'PENDING; no compile/link/runtime/Editor claim','consumer_wrapper_sha256':hashlib.sha256((repo/'Gems/WhiteBox/3rdParty/FindManifold.cmake').read_bytes()).hexdigest()}
(out/'clipper2-audit-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
print(json.dumps(summary,indent=2))

import hashlib,json,pathlib,re
out=pathlib.Path(r'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openmesh-audit')
root=pathlib.Path(r'F:\JericoEngine\Dependencies\OpenMesh-11.0.0')
repo=pathlib.Path(r'F:\JericoEngine\JericoEngine')
manifest=json.loads((out/'source-manifest.json').read_text(encoding='utf-8'))
for item in manifest:
 p=root.joinpath(*pathlib.PurePosixPath(item['path']).parts)
 if hashlib.sha256(p.read_bytes()).hexdigest()!=item['sha256']: raise SystemExit('Source changed: '+item['path'])
patterns={
 'fetchcontent':r'\bFetchContent(?:_\w+)?\b',
 'externalproject':r'\bExternalProject(?:_\w+)?\b',
 'file_download':r'\bfile\s*\(\s*DOWNLOAD\b',
 'execute_process':r'\bexecute_process\s*\(',
 'git_acquisition':r'\bgit\s+(?:clone|submodule)\b',
 'submodule_reference':r'\bsubmodule\b',
}
files=[];hits={k:[] for k in patterns}
for p in root.rglob('*'):
 if p.is_file() and (p.name.lower().startswith('cmakelists.txt') or '.cmake' in p.name.lower()):
  text=p.read_text(encoding='utf-8',errors='replace')
  rel=str(p.relative_to(root)).replace('\\','/')
  files.append({'path':rel,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size})
  for i,line in enumerate(text.splitlines(),1):
   for key,pattern in patterns.items():
    if re.search(pattern,line,re.I): hits[key].append({'path':rel,'line':i,'text':line.strip()})
(out/'cmake-closure-audit.json').write_text(json.dumps({'scope':'static inspection only; upstream CMake not executed','files_inspected':files,'patterns':patterns,'hits':hits,'reachable_downloads_with_current_wrapper_options':[],'conditional_download':'src/Unittests/CMakeLists.txt FetchContent googletest, OPENMESH_BUILD_UNIT_TESTS ON only; root adds Unittests only when CMAKE_PROJECT_NAME matches OpenMesh','bundled_cmake_library':'complete files in archive; no Git/submodule acquisition required','git_metadata':{'gitmodules_present':(root/'.gitmodules').exists(),'cmake_library_git_text':(root/'cmake-library/.git').read_text().strip(),'interpretation':'regular inert metadata file pointing inside source root at .git/modules/cmake-library; referenced directory absent, no Git invocation in inspected CMake scripts'}},indent=2),encoding='utf-8')
license_paths=['LICENSE','src/OpenMesh/Tools/Utils/getopt.c']
licenses=[]
for rel in license_paths:
 p=root/rel
 licenses.append({'path':rel,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'text':p.read_text() if rel=='LICENSE' else p.read_text().split('*/',1)[0]+'*/'})
(out/'license-notices.json').write_text(json.dumps({'root_license':'BSD-3-Clause; RWTH-Aachen University','additional_notice':'getopt.c contains Berkeley four-clause BSD notice, including advertising acknowledgement; Windows OpenMeshTools appends this file to compiled sources','cmake_library':'bundled under source root; no separate license/notice file located in its tree; preserve complete source/headers and root LICENSE, no blanket legal redistribution acceptance','notices':licenses},indent=2),encoding='utf-8')
summary={
 'scope':'audited acquisition and byte-preserving source extraction only; no Git/CMake/build/test/upstream script execution',
 'source_url':'https://www.graphics.rwth-aachen.de/media/openmesh_static/Releases/11.0/OpenMesh-11.0.0.tar.bz2',
 'head_get_status':200,'redirects':0,'credentials':'none','archive':r'F:\JericoEngine\Dependencies\archives\OpenMesh-11.0.0.tar.bz2',
 'archive_bytes':5867933,'archive_sha256':'9d22e65bdd6a125ac2043350a019ec4346ea83922cafdf47e125a03c16f6fa07',
 'source_root':str(root),'version_from_CMakeLists':'11.0.0','git_revision':'not claimed for official release tar; Git fallback not used',
 'members':710,'files':611,'directories':99,'payload_bytes':14703339,'links':0,'reparse_points':0,'collisions':0,'all_source_hashes_match':True,
 'licenses':'root BSD-3-Clause plus Berkeley four-clause BSD notice in Windows OpenMeshTools getopt.c; preserve notices; redistribution acceptance remains parent/legal scope',
 'cmake_files_inspected':len(files),
 'current_wrapper_options':{'OPENMESH_BUILD_SHARED':False,'OPENMESH_DOCS':False,'OPENMESH_BUILD_UNIT_TESTS':False,'DISABLE_QMAKE_BUILD':True,'BUILD_APPS':False,'VCI_NO_LIBRARY_INSTALL':True,'VCI_COMMON_DO_NOT_COPY_POST_BUILD':True},
 'transitive_closure':{'bundled':'cmake-library fully included','active_windows_library_targets':['OpenMeshCore STATIC','OpenMeshTools STATIC -> OpenMeshCore'],'disabled':'apps Qt/OpenGL, docs Doxygen/LATEX, tests GoogleTest/Eigen3','benchmark':'OPENMESH_BENCHMARK_DIR must remain unset/empty; enabling it adds caller-supplied external source','core_optional_header':'EigenVectorT.hh is an optional adapter including Eigen, not a library source dependency from current CMake','local_search_side_effect':'VCIPaths probes local C:/libs D:/libs E:/libs R:/libs; no acquisition; current library path has no dependency finder invocation'},
 'minimum_root_input':r'-DFETCHCONTENT_SOURCE_DIR_OPENMESH:PATH=F:/JericoEngine/Dependencies/OpenMesh-11.0.0',
 'wrapper_change_required_for_current_options':False,
 'parent_next_action':'Freeze explicit local source override with existing disconnected/no-server guards; Luna sole owner root configure/build validation; preserve 611 source hashes; inspect actual target/options/cache before accepting integration',
 'consumer_wrapper_sha256':hashlib.sha256((repo/'Gems/WhiteBox/3rdParty/FindOpenMesh.cmake').read_bytes()).hexdigest(),
 'product_acceptance':'PENDING; acquisition does not prove compilation, linkage, Editor, BSP material, Undo or runtime',
}
(out/'openmesh-audit-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
print(json.dumps({k:summary[k] for k in ('archive_bytes','archive_sha256','source_root','files','all_source_hashes_match','cmake_files_inspected','minimum_root_input','wrapper_change_required_for_current_options','consumer_wrapper_sha256')},indent=2))
print('Acquisition patterns: '+json.dumps({key:len(value) for key,value in hits.items()}))

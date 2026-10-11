"""Static source/recipe receipts and freeze; this is not a build or behavior test."""
import hashlib, json
from pathlib import Path
OUT=Path(__file__).resolve().parent
ENGINE=Path('F:/JericoEngine/JericoEngine')
ROOT=ENGINE/'cmake/Tools/LocalPackages/LZ4'
SNAPSHOT=Path('F:/JericoEngine/Dependencies/lz4-1.9.4')
ARCHIVE=Path('F:/JericoEngine/Dependencies/archives/lz4-v1.9.4.tar.gz')
def digest(path):
    body=path.read_bytes()
    return {'path':str(path),'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest()}
def save(name,value):
    with (OUT/name).open('x',encoding='utf-8') as file: file.write(json.dumps(value,indent=2)+'\n')
files=sorted(path for path in ROOT.rglob('*') if path.is_file())
if len(files)!=7: raise ValueError('Unexpected candidate file count')
for path in files:
    body=path.read_bytes()
    if body.startswith(b'\xef\xbb\xbf'): raise ValueError('Unexpected BOM')
    # New files match the accepted neighboring Windows producer style.
    path.write_bytes(body.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n'))
    body=path.read_bytes()
    if b'\n' in body.replace(b'\r\n',b''): raise ValueError('Mixed endings')
source=json.loads((OUT/'source-extraction.json').read_text())
for row in source['entries']:
    path=SNAPSHOT/row['relative_path']
    if digest(path)['sha256']!=row['sha256']: raise ValueError('Snapshot changed')
archive=digest(ARCHIVE)
if archive['sha256']!='0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b': raise ValueError('Archive changed')
dependencies=[ENGINE/'cmake/Tools/LocalPackages/JECreateLocalPackage.cmake',
              ENGINE/'cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake',
              ENGINE/'LICENSE.txt',ENGINE/'LICENSE_APACHE2.TXT',ENGINE/'LICENSE_MIT.TXT',ARCHIVE]
inputs=[digest(path) for path in files+dependencies]
candidate=[digest(path) for path in files]
aggregate=hashlib.sha256(''.join(row['path']+' '+row['sha256']+'\n' for row in inputs).encode()).hexdigest()
save('source-freeze.json',{'schemaVersion':1,'inputs':inputs,'input_count':len(inputs),'aggregate_sha256':aggregate,
                         'scope':'7 new LZ4 candidate files; 6 unchanged shared/license/archive inputs',
                         'build_executed':False,'consumer_executed':False})
notices=[]
for name in ('lz4.c','lz4.h','lz4hc.c','lz4hc.h','lz4frame.c','lz4frame.h','lz4frame_static.h','xxhash.c','xxhash.h'):
    path=SNAPSHOT/'lib'/name
    text=path.read_text(encoding='utf-8')
    if 'BSD 2-Clause' not in text or 'Copyright' not in text: raise ValueError('Expected notice missing: '+name)
    notices.append(digest(path))
save('source-notices.json',{'runtime_library_license':'BSD-2-Clause','components':'LZ4 core/HC/frame and embedded xxHash',
                           'full_license':digest(SNAPSHOT/'lib/LICENSE'),'upstream_scope_notice':digest(SNAPSHOT/'LICENSE'),
                           'upstream_cmake_notice':digest(SNAPSHOT/'build/cmake/CMakeLists.txt'),
                           'compiled_library_notices':notices,'o3de_glue_licenses':[digest(path) for path in dependencies[2:5]],
                           'excluded':'GPL CLI/tests/examples are not built or packaged; tar retained as separate source snapshot',
                           'legal_acceptance':'Not decided by this static audit'})
save('audit-summary.json',{
    'schemaVersion':1,'source':{'repository':'lz4/lz4','tag':'v1.9.4','commit':'5ff839680134437dbf4678f3d0c7b371d84f4964',
                              'url':'https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4','archive':archive,'snapshot':str(SNAPSHOT),
                              'members':244,'regular_files':187,'regular_bytes':1668409,'links':0,
                              'git_blob_correspondence':'154 files byte-exact;33 declared CRLF exports with exact normalized Git blobs',
                              'compiled_source_headers_cmake_byte_exact':True},
    'original':{'recipe_repository':'o3de/3p-package-source','reviewed_commit':'70f0b7b5257bf7cf935bf1cfd270e1b53ccead18',
                'historical_rev1_recipe_commit':'68e97674712afd8c5e5c99608b35442da579163a',
                'historical_package':'lz4-1.9.4-rev1-windows','historical_binary_sha256':'aa7d4e8f6424fad7fdc0304fecd83c1183886cc4c7b38ba014c85f43d036e550',
                'configurations':['Release'],'shared':False,'generator':'Visual Studio 16','CRT':'Default CMake Release DLL runtime; no CRT override in original recipe',
                'debug_archive_provided':False,'imported_target':'3rdParty::lz4','imported_location':'lz4/lib/lz4.lib',
                'headers':['lz4/include/lz4.h','lz4/include/lz4hc.h','lz4/include/lz4frame.h'],
                'current_recipe_difference':'rev2 adds Linux/Mac-arm64/Emscripten/compatibility settings; Windows Release/static/Find unchanged',
                'cli':'Original upstream CLI defaults ON and generic driver builds/installs all targets; no engine CLI caller found'},
    'candidate':{'files':candidate,'identity':'lz4-1.9.4-je-rev1-windows','toolset':'VS2022/v143 Windows x64','CRT':'MultiThreadedDLL (/MD)',
                 'configuration':'Release','upstream_target':'lz4_static','core':True,'HC':True,'frame':True,'xxHash':True,
                 'source_patches':False,'cli':False,'legacy_cli':False,'upstream_install':False,'CPack':False,
                 'consumer_cases':['blocks default+HC roundtrip','default ZipDir frame roundtrip','fragmented frame decode','short destinations/invalid magic/incomplete truncated frame'],
                 'consumer_configurations':['Release','Debug (same Release imported archive)'],
                 'guards':['explicit local absolute paths/no UNC/URI/symlink ancestors','new outputs beneath checkout/build',
                           'exact official source URL/commit/SHA256 before extraction/configure','missing input rejected','no implicit acquisition/global install']},
    'consumers':[{'path':'Code/Framework/AzFramework/CMakeLists.txt:41','usage':'PUBLIC 3rdParty::lz4'},
                 {'path':'Code/Framework/AzFramework/AzFramework/Archive/ZipDirCache.cpp:218','usage':'LZ4F_compressFrameBound'},
                 {'path':'Code/Framework/AzFramework/AzFramework/Archive/ZipDirStructures.cpp:521-547,618','usage':'frame compression/context/decompression/error/free'},
                 {'path':'Gems/Compression/Code/Source/Tools/CompressorLZ4Impl.cpp','usage':'LZ4_compressBound/compress_default'},
                 {'path':'Gems/Compression/Code/Source/Clients/DecompressorLZ4Impl.cpp','usage':'LZ4_decompress_safe'},
                 {'path':'Gems/MultiplayerCompression','usage':'historical excluded consumer; not activated by this dependency recipe'}],
    'validation':{'state':'IMPLEMENTED NOT VERIFIED','CMake_compiler_tests_executed':False,'owner':'Luna assigned by parent',
                  'pending':['producer guards','native producer','tar/hash/content inspection','extracted archive consumer Release+Debug','parent catalog/root integration'],
                  'limits':['No historical binary ABI/performance assertion','No Editor frame/runtime or legal acceptance','No catalog/docs/WhiteBox/helper changes']},
    'receipts':['destination-receipts.json','recipe-destination-receipts.json','recipe-history-receipts.json','source-acquisition.json',
                'source-tar-audit.json','source-extraction.json','archive-difference.json','source-notices.json','source-freeze.json'],
    'freeze_aggregate_sha256':aggregate})
print(json.dumps({'candidate_files':len(candidate),'freeze_inputs':len(inputs),'aggregate_sha256':aggregate,
                  'freeze_json_sha256':digest(OUT/'source-freeze.json')['sha256'],'summary_sha256':digest(OUT/'audit-summary.json')['sha256']}))

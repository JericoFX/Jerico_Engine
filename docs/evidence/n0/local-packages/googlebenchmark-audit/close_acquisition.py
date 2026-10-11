"""Static provenance and input freeze only; no native project execution."""
import hashlib
import json
from pathlib import Path
OUT = Path(__file__).resolve().parent
SOURCE = Path('F:/JericoEngine/Dependencies/googlebenchmark-1.7.0')
WRAPPER = Path('F:/JericoEngine/JericoEngine/cmake/3rdParty/FindGoogleBenchmark.cmake')
extraction = json.loads((OUT/'source-extraction.json').read_text())
freeze = []
for entry in extraction['entries']:
    path = SOURCE/entry['relative_path']
    data = path.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    if sha!=entry['sha256'] or len(data)!=entry['bytes']:
        raise ValueError('Acquired source changed during static inspection')
    freeze.append({'path':str(path),'sha256':sha,'bytes':len(data)})
data = WRAPPER.read_bytes()
change = json.loads((OUT/'wrapper-guard-change.json').read_text())
if hashlib.sha256(data).hexdigest()!=change['after_sha256']:
    raise ValueError('Wrapper changed during static inspection')
freeze.append({'path':str(WRAPPER),'sha256':change['after_sha256'],'bytes':len(data)})
notices = []
for folder in (SOURCE/'include',SOURCE/'src'):
    for path in sorted(folder.rglob('*')):
        if path.is_file() and path.suffix in ('.cc','.h'):
            text = path.read_text(encoding='utf-8')
            lines = text.splitlines()
            attribution = [{'line':index+1,'text':line} for index,line in enumerate(lines)
                           if any(term in line.lower() for term in ('copyright','licensed under','license','copied from','trademark'))]
            notices.append({'path':str(path.relative_to(SOURCE)).replace('\\','/'),
                            'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                            'leading_source_text':'\n'.join(lines[:16]),'attribution_lines':attribution})
(OUT/'source-notices.json').write_text(json.dumps(notices,indent=2)+'\n',encoding='utf-8')
(OUT/'source-freeze.json').write_text(json.dumps({'inputs':freeze,'count':len(freeze),'static_only':True},indent=2)+'\n',encoding='utf-8')
summary = {'status':'source acquired, wrapper guard integrated; configure/build validation pending Luna',
    'acquisition':json.loads((OUT/'source-acquisition.json').read_text()),
    'tar':{key:value for key,value in json.loads((OUT/'source-tar-audit.json').read_text()).items() if key!='entries'},
    'snapshot':str(SOURCE),'source_all_sha256_checked':True,'all_source_commit_blobs_match':True,
    'source_patch':False,'source_writes':'None performed after extraction; upstream generated files use binary dirs. Wrapper now blocks upstream three Git queries/index refresh by disabling FindGit and shadowing inherited/cache GIT_EXECUTABLE with empty normal variable.',
    'guard':change,'license':'Apache-2.0; full LICENSE, AUTHORS, CONTRIBUTORS and all source notices intact; no separate NOTICE in pinned tree',
    'current_wrapper_features_preserved':True,'no_download_paths_active_with_wrapper_options':'GTest ExternalProject download remains behind disabled testing/gtests/bundled/download options; no active upstream network commands found in current CMake path.',
    'source_override':'-DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK:PATH=F:/JericoEngine/Dependencies/googlebenchmark-1.7.0',
    'freeze_inputs':len(freeze),'source_notices_files':len(notices),
    'limits':['No CMake, build, native probe, benchmark, tests, install or Git commands executed.',
              'Effective inherited-Git guard, engine integration, ABI, runtime/performance and redistribution acceptance remain pending parent/Luna.',
              'Earlier audit-summary.json records the pre-acquisition proposal; acquisition-summary.json and wrapper-guard-change.json reflect this later authorized acquisition and guard.']}
(OUT/'acquisition-summary.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'freeze_inputs':len(freeze),'source_notices_files':len(notices),
                 'summary_sha256':hashlib.sha256((OUT/'acquisition-summary.json').read_bytes()).hexdigest(),
                 'freeze_sha256':hashlib.sha256((OUT/'source-freeze.json').read_bytes()).hexdigest()}))

"""Evidence summary only; does not execute the audited project."""
import datetime
import hashlib
import json
import os
from pathlib import Path
OUT = Path(__file__).resolve().parent
ROOT = Path('F:/JericoEngine/JericoEngine')
wrapper = ROOT/'cmake/3rdParty/FindGoogleBenchmark.cmake'
tree = json.loads((OUT/'tree.json').read_text())
source_url = 'https://codeload.github.com/google/benchmark/tar.gz/refs/tags/v1.7.0'
flags = {name: 'OFF' for name in ('BUILD_SHARED_LIBS','BENCHMARK_DOWNLOAD_DEPENDENCIES',
    'BENCHMARK_ENABLE_ASSEMBLY_TESTS','BENCHMARK_ENABLE_DOXYGEN','BENCHMARK_ENABLE_GTEST_TESTS',
    'BENCHMARK_ENABLE_INSTALL','BENCHMARK_ENABLE_LIBPFM','BENCHMARK_ENABLE_TESTING',
    'BENCHMARK_ENABLE_WERROR','BENCHMARK_FORCE_WERROR','BENCHMARK_INSTALL_DOCS','BENCHMARK_USE_BUNDLED_GTEST')}
result = {
 'status': 'primary source text and destination HEAD audit; archive acquisition and configure pending',
 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
 'consumer': {'path': str(wrapper),'bytes': wrapper.stat().st_size,'sha256':hashlib.sha256(wrapper.read_bytes()).hexdigest(),
    'source_mode': 'FetchContent_MakeAvailable runs upstream CMake; EXCLUDE_FROM_ALL and SYSTEM, not NO_ADD_SUBDIR',
    'version': 'v1.7.0','git_commit':'361e8d1cfe0c6c36d30b39f1b61302ece5507320',
    'url': 'https://github.com/google/benchmark/archive/refs/tags/v1.7.0.tar.gz',
    'source_archive_expected_sha256':'3aff99169fa8bdee356eaa1f691e835a6e57b1efeadb8a0f9f228531158246ac',
    'archive_sha256_verified':False,'options':flags,'CMAKE_POLICY_VERSION_MINIMUM':'3.10',
    'aliases':['benchmark::benchmark','benchmark::benchmark_main','3rdParty::GoogleBenchmark'],
    'engine_interface_defines':['HAVE_BENCHMARK','BENCHMARK_STATIC_DEFINE'],
    'targets': 'static benchmark and benchmark_main; engine interface links benchmark::benchmark',
    'windows_link': ['Threads::Threads','shlwapi'],
    'includes':['source/include','binary/include'],
    'installed_engine_layout': ['include/googlebenchmark/benchmark/*.h','include/googlebenchmark/LICENSE','lib/Windows/<configuration>/benchmark.lib'],
    'required_by':['Code/Framework/AzTest/CMakeLists.txt:33','Code/Framework/AzCore/CMakeLists.txt:147'],
    'reason_required':'AzTest target setup is explicitly independent of whether ordinary unit tests are enabled.'},
 'official_tag': {'url':'https://api.github.com/repos/google/benchmark/git/ref/tags/v1.7.0',
                  'commit_matches_consumer':True,'tree_truncated':tree['truncated'],
                  'gitlinks':[row for row in tree['tree'] if row['type']=='commit']},
 'destination': {'direct_url':source_url,'original_url_head':302,'direct_head':200,
    'automatic_redirects':False,'authorization':False,'content_length':None,
    'etag_is_not_the_expected_archive_sha256':True,'archive_get_performed':False,
    'next_acquisition':'Parent approval, repeat direct HEAD, one bounded direct GET, full SHA256 exact, tar paths/types/links/collisions audit before extraction. No alternate destination on failure.'},
 'cmake_closure': {
    'project':'benchmark 1.7.0 CXX; minimum 3.5.1, CMP0077 NEW honors importing normal option overrides',
    'active_local_checks':'Compiler flag checks, regex and steady_clock try_run/try_compile; Threads and optional rt check. Native probes run locally at configure; audit did not execute them.',
    'write_locations':'Active src/CMakeLists generates config/version/pkg-config/export files in PROJECT_BINARY_DIR. Feature checks build under CMAKE_BINARY_DIR. No active CMake source-file write found except Git index refresh risk below.',
    'inactive_download':'cmake/GoogleTest.cmake.in ExternalProject googletest release-1.11.0 Git download is nested behind BENCHMARK_ENABLE_TESTING && BENCHMARK_ENABLE_GTEST_TESTS && BENCHMARK_USE_BUNDLED_GTEST, then ALLOW_DOWNLOADING_GOOGLETEST. Wrapper disables all four.',
    'other_disabled_paths':'libpfm, Doxygen, installation and assembly scripts are disabled. Windows MSVC assembly default returns before searching FileCheck.',
    'unconditional_git': {'path':'cmake/GetGitVersion.cmake','calls':['find_package(Git)','git describe --tags --match v[0-9]*.[0-9]*.[0-9]* --abbrev=8','git update-index -q --refresh','git diff-index --name-only HEAD --'],
      'working_directory':'PROJECT_SOURCE_DIR','risk':'Source under checkout/build inherits engine Git repository and can refresh its index. Outside-repository snapshot removes that ancestor but ambient GIT_DIR/GIT_WORK_TREE could still redirect Git.',
      'proposed_minimum_guard':'Parent-owned wrapper block set(CMAKE_DISABLE_FIND_PACKAGE_Git ON) before FetchContent_MakeAvailable; upstream no-Git branch returns 0.0.0 and root project falls back to benchmark_VERSION 1.7.0. Preserve all options and library features. Not implemented or executed.',
      'bounded_ancestor_inspection':[{'path':p,'exists':Path(p).exists()} for p in ('F:/JericoEngine/Dependencies/.git','F:/JericoEngine/.git','F:/.git')],
      'ambient_git_variable_presence':{name:name in os.environ for name in ('GIT_DIR','GIT_WORK_TREE','GIT_INDEX_FILE')}}},
 'source_override_proposal':'-DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK:PATH=F:/JericoEngine/Dependencies/googlebenchmark-1.7.0',
 'licenses': {'declared':'Apache-2.0','full_license':'upstream/LICENSE','license_sha256':'cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30',
    'preserve':['LICENSE','AUTHORS','CONTRIBUTORS','all native source attribution notices'],
    'notice_file_in_commit_tree':False,'limitations':'No legal redistribution acceptance, no ABI/performance/product or engine build claim. Full acquired archive source headers still require payload audit.'},
 'primary_text_inputs':json.loads((OUT/'text-inputs.json').read_text())+json.loads((OUT/'additional-text-inputs.json').read_text()),
 'limits':['No source/configuration/documentation/Git changes. No CMake/build/test/install. No GoogleBenchmark archive GET or extraction.',
           'Text inspection establishes proposed paths/guards; only Luna can validate effective CMake and engine integration after parent freezes inputs.']}
(OUT/'audit-summary.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'summary_sha256':hashlib.sha256((OUT/'audit-summary.json').read_bytes()).hexdigest(),'text_inputs':len(result['primary_text_inputs'])}))

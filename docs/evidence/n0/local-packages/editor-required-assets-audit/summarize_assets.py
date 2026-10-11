"""Build the review manifest from static inspection receipts; no network or engine execution."""
import hashlib
import json
import re
import urllib.parse
from pathlib import Path

ROOT = Path('F:/JericoEngine/JericoEngine')
OUT = ROOT / 'build/n0-validation/local-packages/editor-required-assets-audit'
def read(name):
    return json.loads((OUT / name).read_text(encoding='utf-8'))
def write(name, data):
    (OUT / name).write_text(json.dumps(data, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
def source(path):
    data = (ROOT / path).read_bytes()
    return {'path': path, 'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)}

native = read('native-resource-inputs.json')
qrc = read('editor-qrc-inputs.json')
fonts = read('font-inputs.json')
runtime = read('runtime-asset-inputs.json')
source_notices = []
for item in runtime['static_source_closure']:
    if item['exists'] and not item.get('lfs_pointer'):
        lines = (ROOT / item['path']).read_text(encoding='utf-8-sig').splitlines()
        found = [{'line': index+1, 'text': line} for index, line in enumerate(lines)
                 if re.search(r'SPDX-License-Identifier|Copyright|Permission is hereby|LICENSE|Redistribution and use|All rights reserved', line, re.I)]
        source_notices.append({'path': item['path'], 'sha256': item['file_sha256'], 'notice_lines': found,
                               'repository_default_only': not bool(found)})
write('shader-source-notices.json', {'scope': 'Static source notice locations only; full original files and independent license texts remain authoritative. Binary metadata does not certify embedded notices or redistribution.',
                                   'files': source_notices})
heads = read('github-payload-head-audit.json')
commit = heads[0]['commit']
head_by_path = {row['path']: row for row in heads}
items = {}
def add(item, category, consumer, priority):
    if not item.get('lfs_pointer'):
        return
    relative = item['path']
    row = items.setdefault(relative, {'path': relative, 'payload_sha256': item['payload_sha256'],
        'payload_bytes': item['payload_bytes'], 'pointer_sha256': item['file_sha256'],
        'github_commit': commit, 'payload_candidate_url': 'https://media.githubusercontent.com/media/o3de/o3de/'+commit+'/'+urllib.parse.quote(relative),
        'metadata_candidate_url': 'https://api.github.com/repos/o3de/o3de/contents/'+urllib.parse.quote(relative)+'?ref='+commit,
        'categories': [], 'consumers': [], 'priorities': [], 'payload_downloaded': False})
    for key, value in [('categories', category), ('consumers', consumer), ('priorities', priority)]:
        if value not in row[key]:
            row[key].append(value)
    row['destination_audit'] = head_by_path.get(relative, {'status': 'CANDIDATE_UNPROBED: parent must audit direct HEAD/metadata before GET'})

for rc in native['rc_consumers']:
    for ref in rc['references']:
        if ref['input']:
            add(ref['input'], 'native_windows_rc', rc['consumer']['path'], 'Binary format required by native resource compiler')
for row in native['resources']:
    if row['path'] in ['Code/LauncherUnified/Resources/GameSDK.ico', 'Templates/ScriptOnlyProject/Template/Resources/Splash.bmp',
                       'Templates/ScriptOnlyProject/Template/Resources/GameSDK.ico']:
        add(row, 'launcher_windows_rc', 'Code/LauncherUnified/Platform/Windows/launcher_project_windows.cmake',
            'One actual ICO and BMP pair needed; parent may copy exact template payloads into project Resources')
for row in qrc:
    if row['editor_cmake_listed'] or row['input']['path'].endswith('.ttf'):
        add(row['input'], 'qt_rcc_encoded_input', row['consumer'],
            'RCC input bytes for active listed QRC; pointer bytes are not usable image/font payloads at runtime')
for row in fonts['engine_fonts']:
    if row['path'].endswith(('.font', '.ttf')):
        add(row, 'atom_font_runtime', 'Gems/AtomLyIntegration/AtomFont/Code/Source/AtomFontSystemComponent.cpp',
            'Default/default-ui loaded on system initialization; console/hud retained separately as available original descriptors')
for row in runtime['feature_processor_binary_inputs']:
    add(row, 'atom_feature_processor_default', 'ImageBasedLightFeatureProcessor/SkyBoxFeatureProcessor/LtcCommon/SMAACommon/DepthOfFieldPencilMap',
        'IBL/SkyBox/LTC activation defaults retained without authored components; SMAA/DOF settings-dependent requirements')
for row in runtime['default_grid_and_whitebox_uv_inventory']:
    add(row, 'legacy_material_or_scanned_asset', 'Gem WhiteBoxDefault.material/default_grid.material or enabled scan-folder source',
        'Not directly referenced by the project constant material; AP scan/job selection remains unverified')

duplicate_groups = {}
for item in items.values():
    duplicate_groups.setdefault(item['payload_sha256'], []).append(item['path'])
write('acquisition-review-manifest.json', {'schema': 1, 'official_repo': 'https://github.com/o3de/o3de',
    'github_commit': commit, 'acquisition_authorized': False, 'items': sorted(items.values(), key=lambda row: row['path']),
    'exact_duplicate_payload_groups': [ {'sha256': sha, 'paths': paths} for sha, paths in duplicate_groups.items() if len(paths)>1 ],
    'font_local_original_recovery': fonts['local_original_candidates'],
    'font_relationships': [
        {'descriptor': 'Assets/Engine/Fonts/default.font', 'xml': 'Assets/Engine/Fonts/default.xml', 'font': 'Assets/Engine/Fonts/VeraMono.ttf'},
        {'descriptor': 'Assets/Engine/Fonts/default-ui.font', 'xml': 'Assets/Engine/Fonts/default-ui.xml', 'font': 'Assets/Engine/Fonts/Vera.ttf'},
        {'descriptor': 'Assets/Engine/Fonts/console.font', 'xml': 'Assets/Engine/Fonts/console.xml', 'font': 'Assets/Engine/Fonts/VeraMono.ttf'},
        {'descriptor': 'Assets/Engine/Fonts/hud.font', 'xml': 'Assets/Engine/Fonts/hud.xml', 'font': 'Assets/Engine/Fonts/VeraMono.ttf'}],
    'license_and_notice_sources': [source(path) for path in ['LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT',
        'Assets/Engine/Fonts/Vera_COPYRIGHT.TXT','Assets/Engine/Fonts/Vera_RELEASENOTES.TXT',
        'Assets/Editor/Fonts/Open_Sans/LICENSE.txt',
        'Code/Framework/AzQtComponents/AzQtComponents/Components/Fonts/Open_Sans/LICENSE.txt']],
    'license_limits': ['Vera 1.10 has the separate original Bitstream license and trademark/name conditions; preserve the full copyright notice.',
        'Open Sans embedded font folder supplies Apache-2.0 license text; actual font payload notices/versions await inspection.',
        'Icons and textures have repository default Apache-2.0 OR MIT notices; LFS pointer metadata cannot certify original artist/trademark or embedded notice rights.',
        'No redistribution or legal acceptance is decided by this acquisition manifest.'],
    'manual_copy_guards': ['Do not use .lfsconfig, git-lfs pull, AWS/S3/CloudFront or redirects.',
        'Parent must audit unprobed direct destinations; obtain payload only after authorization and verify full SHA256 and byte size before any source copy.',
        'Store downloads in a new Dependencies destination first. Source replacement must compare the exact current pointer SHA256, OID and size against this manifest.',
        'For existing non-pointer files, refuse overwrite. For new project Resources paths, refuse existing paths and keep scope explicit.',
        'The four .font candidates require only the verified existing XML CRLF-to-LF transform, with full output SHA256/size equality; no font-generation tool or substitute content.']})

active_qrc = [row for row in qrc if row['editor_cmake_listed']]
write('audit-summary.json', {'status': 'STATIC AUDIT ONLY; NO PAYLOAD GET/MATERIALIZATION/EDITOR/BUILD/TEST',
    'resource_inventory': {'ico_bmp_files': len(native['resources']),
                           'direct_native_rc_inputs': [ref['input']['path'] for rc in native['rc_consumers'] for ref in rc['references'] if ref['input']]},
    'qrc_scope': {'all_entries': len(qrc), 'active_editor_entries': len(active_qrc),
                  'active_editor_lfs_entries': sum(row['input'].get('lfs_pointer', False) for row in active_qrc),
                  'active_editor_missing_entries': sum(not row['input']['exists'] for row in active_qrc),
                  'embedded_qt_font_entries': len(fonts['editor_embedded_fonts']),
                  'excluded_missing_qrc_note': 'InfoBar.qrc/i18n_Translations.qrc are not in current Editor file lists. No claim their absent source SVG/QM blocks this target.'},
    'static_atom_sources': len(runtime['static_source_closure']),
    'static_atom_pointer_sources': sum(row.get('lfs_pointer', False) for row in runtime['static_source_closure']),
    'static_atom_missing_sources': sum(not row['exists'] for row in runtime['static_source_closure']),
    'static_unresolved_edges': runtime['unresolved_references'],
    'closure_findings': [
        'StandardPBR remains required: imported property groups, native material builder, Lua functors, MainPipeline and LowEndPipeline registries, generated shader templates and shader/include sources.',
        'Atom default MainRenderPipeline/MainPipeline/pass-template graph and dynamically registered CopyPassTemplate remain required; static graph includes disabled paths conservatively.',
        'AtomFont critical shader is in AtomLyIntegration/AtomBridge/Assets/Shaders/SimpleTextured.shader, not AtomFont/Assets.',
        'ImageBasedLightFeatureProcessor and SkyBoxFeatureProcessor Activate always load default maps even without scene components, explicitly to avoid Vulkan TDR in current upstream comments.',
        'default_iblglobalcm.dds produces both _iblspecular and _ibldiffuse streaming-image outputs via ImageConvert.cpp. It is a single source payload, identical to default_skyboxcm.dds by LFS SHA/size.',
        'Quad/Polygon feature processor activation loads LTC matrix/amplification images into the scene SRG. SMAA AreaTex/SearchTex and DOF pencil map are settings-dependent.',
        'ImageSystem generates its one-pixel system colors through CreateFromCpuData; these do not require a fabricated white/black TIFF.',
        'Project constant WhiteBox material has no default_grid parent or WhiteBoxUV texture reference. Legacy default_grid three TIFFs and WhiteBox UV three TIFFs still exist in enabled scan folders; AP jobs/product priority require runtime evidence.',
        'Native RC ICO/BMP is a compile-resource requirement; Qt QRC is byte embedding plus later image/font decoding. Listed CMake files alone are not proof an ICO/BMP is a native compiled resource.'],
    'head_audit': [{'path': row['path'], 'status': row['payload_head']['status'],
                    'pointer_match': row['pointer_matches_local_bytes'], 'content_length': row['payload_head']['headers']['Content-Length'],
                    'etag': row['payload_head']['headers']['ETag'], 'no_auth': True, 'no_redirect': True} for row in heads],
    'manifest_items': len(items), 'font_recovery_matches': sum(row['exact_pointer_match'] for row in fonts['local_original_candidates']),
    'limits': ['No binary has been acquired or imported; HEAD ETag/length is corroboration, not a GET payload hash verification.',
        'Static closure contains 529 existing regular source files and zero unresolved references in the bounded Atom/material/pass roots; this is not complete dynamic AP/product or all Editor dependency Qt-resource closure.',
        'CMake/Editor/AP execution, shader compilers, product priority, font parsing, image formats, frames, collision and DX12/Vulkan acceptance remain assigned to Luna/parent.',
        'JEEditorFixture20/Python/Lua source freezes are preserved. No alternate toolchain icon/font substitution is needed or proposed while exact official payload destinations are available.']})
print(json.dumps({'manifest_items': len(items), 'summary': str(OUT/'audit-summary.json')}))

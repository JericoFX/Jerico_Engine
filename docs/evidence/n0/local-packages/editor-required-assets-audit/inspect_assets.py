"""Static file/hash/reference audit only. No engine imports, builds or asset downloads."""
import base64
import hashlib
import json
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path('F:/JericoEngine/JericoEngine')
OUT = ROOT / 'build/n0-validation/local-packages/editor-required-assets-audit'

def digest(data):
    return hashlib.sha256(data).hexdigest()

def info(path):
    path = Path(path)
    item = {'path': path.relative_to(ROOT).as_posix(), 'exists': path.is_file()}
    if path.is_file():
        data = path.read_bytes()
        item.update(bytes=len(data), file_sha256=digest(data))
        match = re.fullmatch(rb'version https://git-lfs.github.com/spec/v1\r?\noid sha256:([a-f0-9]{64})\r?\nsize (\d+)\r?\n?', data)
        item['lfs_pointer'] = bool(match)
        if match:
            item.update(payload_sha256=match[1].decode(), payload_bytes=int(match[2]))
        else:
            item['signature_hex'] = data[:8].hex()
    return item

def text(path):
    return path.read_text(encoding='utf-8-sig')

def save(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')

resources = []
for folder in ['Code/Editor/res', 'Code/LauncherUnified/Resources',
               'Templates/ScriptOnlyProject/Template/Resources',
               'Code/Tools/AssetProcessor/Platform/Windows', 'Code/Tools/AssetProcessor/AssetBuilder',
               'Code/Tools/AssetProcessor/native/ui/style', 'Code/Tools/ProjectManager/Resources']:
    for path in sorted((ROOT / folder).glob('*')):
        if path.suffix.lower() in ('.ico', '.bmp'):
            resources.append(info(path))

native = []
for rc in ['Code/Editor/EditorCryEdit.rc', 'Code/Editor/CryEdit.rc', 'Code/Editor/res/CryEdit.rc2',
           'Code/LauncherUnified/Platform/Windows/Launcher.rc.in',
           'Code/Tools/AssetProcessor/Platform/Windows/AssetProcessor.rc',
           'Code/Tools/AssetProcessor/Platform/Windows/AssetProcessorBatch.rc',
           'Code/Tools/AssetProcessor/AssetBuilder/AssetBuilder.rc',
           'Code/Tools/ProjectManager/Resources/ProjectManager.rc']:
    path = ROOT / rc
    references = re.findall(r'\b(?:ICON|BITMAP)\s+(?:DISCARDABLE\s+)?"([^"]+)"', text(path))
    entries = []
    for ref in references:
        ref = ref.replace('\\\\', '/').replace('\\', '/')
        entries.append({'literal': ref, 'input': info((path.parent / ref).resolve()) if '@' not in ref else None})
    native.append({'consumer': info(path), 'references': entries})
save('native-resource-inputs.json', {'resources': resources, 'rc_consumers': native})

qrc_inputs = []
listed_qrcs = set()
for filelist in (ROOT / 'Code/Editor').glob('*files.cmake'):
    listed_qrcs.update(re.findall(r'(?m)^\s+([^\s()]+\.qrc)\s*$', text(filelist)))
qrc_paths = list((ROOT / 'Code/Editor').rglob('*.qrc'))
qrc_paths += [ROOT / 'Code/Framework/AzQtComponents/AzQtComponents/Components/resources.qrc']
for qrc in sorted(qrc_paths):
    for element in ET.parse(qrc).iter('file'):
        if element.text:
            path = (qrc.parent / element.text).resolve()
            qrc_inputs.append({'consumer': qrc.relative_to(ROOT).as_posix(), 'alias': element.attrib.get('alias'),
                               'editor_cmake_listed': qrc.relative_to(ROOT / 'Code/Editor').as_posix() in listed_qrcs
                               if qrc.is_relative_to(ROOT / 'Code/Editor') else None,
                               'input': info(path)})
save('editor-qrc-inputs.json', qrc_inputs)

font_inputs = [info(p) for p in sorted((ROOT / 'Assets/Engine/Fonts').iterdir()) if p.is_file()]
font_duplicates = []
for base in ['default', 'default-ui', 'console', 'hud']:
    pointer = info(ROOT / ('Assets/Engine/Fonts/' + base + '.font'))
    xml = ROOT / ('Assets/Engine/Fonts/' + base + '.xml')
    payload = xml.read_bytes().replace(b'\r\n', b'\n')
    font_duplicates.append({'source': info(xml), 'candidate_transform': 'CRLF to LF only',
                            'candidate_bytes': len(payload), 'candidate_sha256': digest(payload),
                            'exact_pointer_match': pointer.get('payload_bytes') == len(payload)
                            and pointer.get('payload_sha256') == digest(payload)})
save('font-inputs.json', {'engine_fonts': font_inputs, 'local_original_candidates': font_duplicates,
     'editor_embedded_fonts': [row for row in qrc_inputs if row['input']['path'].endswith('.ttf')]})

def clean_json(source):
    token = re.compile(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\r\n]*', re.S)
    return json.loads(token.sub(lambda m: m[0] if m[0].startswith('"') else '', source))

def strings(value):
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for child in value.values():
            yield from strings(child)
    elif isinstance(value, list):
        for child in value:
            yield from strings(child)

asset_roots = [ROOT / 'Gems/Atom/Feature/Common/Assets', ROOT / 'Gems/Atom/RPI/Assets',
               ROOT / 'Gems/AtomLyIntegration/AtomBridge/Assets', ROOT / 'Projects/JEEditorFixture/Assets']
pass_aliases = {}
for assets in asset_roots:
    if assets.exists():
        for path in assets.rglob('*.azasset'):
            try:
                data = clean_json(text(path))
            except (ValueError, UnicodeError):
                continue
            for row in data.get('ClassData', {}).get('AssetPaths', []):
                pass_aliases.setdefault(row['Name'], []).append((assets / row['Path']).resolve())

include_roots = [ROOT / 'Projects/JEEditorFixture/ShaderLib', ROOT / 'Gems/Atom/Feature/Common/Assets/ShaderLib',
                 ROOT / 'Gems/Atom/RPI/Assets/ShaderLib', ROOT / 'Gems/Atom/RHI/Assets/ShaderLib', ROOT / 'Gems']
extensions = ('.json', '.materialtype', '.materialpipeline', '.shader', '.shader.template', '.azsli', '.azsl',
              '.srgi', '.lua', '.pass', '.azasset', '.tif', '.dds', '.png')
initial = [ROOT / 'Projects/JEEditorFixture/Assets/materials/WhiteBoxDefault.material',
           asset_roots[0] / 'Materials/Types/StandardPBR.materialtype',
           asset_roots[0] / 'Materials/Pipelines/MainPipeline/MainPipeline.materialpipeline',
           asset_roots[0] / 'Materials/Pipelines/LowEndPipeline/LowEndPipeline.materialpipeline',
           asset_roots[0] / 'Passes/MainRenderPipeline.azasset', asset_roots[0] / 'Passes/MainPipeline.pass',
           ROOT / 'Gems/AtomLyIntegration/AtomBridge/Assets/Shaders/SimpleTextured.shader']
visited = set()
edges, unresolved = [], []
todo = list(initial)
while todo:
    path = todo.pop().resolve()
    if path in visited:
        continue
    visited.add(path)
    if not path.is_file() or info(path).get('lfs_pointer'):
        continue
    source = text(path)
    references = set(re.findall(r'#\s*include\s*[<"]([^>"\r\n]+)', source))
    try:
        data = clean_json(source)
        references.update(s for s in strings(data) if s.lower().endswith(extensions))
        def template_names(node):
            if isinstance(node, dict):
                if 'TemplateName' in node:
                    yield node['TemplateName']
                for item in node.values():
                    yield from template_names(item)
            elif isinstance(node, list):
                for item in node:
                    yield from template_names(item)
        for name in template_names(data):
            if name == 'CopyPassTemplate':
                edges.append({'from': path.relative_to(ROOT).as_posix(), 'template': name,
                              'native_registration': 'Gems/Atom/RPI/Code/Source/RPI.Public/Pass/PassLibrary.cpp:AddCopyPassTemplate'})
                continue
            matches = pass_aliases.get(name, [])
            if len(matches) == 1:
                edges.append({'from': path.relative_to(ROOT).as_posix(), 'template': name,
                              'to': matches[0].relative_to(ROOT).as_posix()})
                todo.extend(matches)
            else:
                unresolved.append({'from': path.relative_to(ROOT).as_posix(), 'template': name,
                                   'reason': 'No unique alias in scoped Atom asset roots'})
    except ValueError:
        pass
    for ref in sorted(references):
        if ref.startswith('@gemroot:Atom_Feature_Common@/'):
            candidates = [ROOT / 'Gems/Atom/Feature/Common' / ref.split('@/', 1)[1]]
        else:
            candidates = [path.parent / ref] + [folder / ref for folder in asset_roots + include_roots]
        candidates = list(dict.fromkeys(p.resolve() for p in candidates if p.is_file()))
        if candidates:
            selected = candidates[0]
            edges.append({'from': path.relative_to(ROOT).as_posix(), 'reference': ref,
                          'to': selected.relative_to(ROOT).as_posix(), 'resolution': 'relative then scoped asset/include roots'})
            todo.append(selected)
        else:
            unresolved.append({'from': path.relative_to(ROOT).as_posix(), 'reference': ref})

runtime_binary = [asset_roots[0] / path for path in [
    'Textures/Default/default_iblglobalcm.dds',
    'Textures/Default/default_skyboxcm.dds', 'Textures/LTC/ltc_mat_lutrgba32f.dds', 'Textures/LTC/ltc_amp_lutrg32f.dds',
    'Textures/PostProcessing/AreaTex.dds', 'Textures/PostProcessing/SearchTex.dds',
    'Textures/PostProcessing/DepthOfField_PencilMap_35mmFilm.bmp']]
historical = list((ROOT / 'Gems/WhiteBox/Assets/textures/uvs').glob('*.tif'))
historical += list((asset_roots[0] / 'Textures/Default').glob('*.tif'))
save('runtime-asset-inputs.json', {'static_source_closure': [info(p) for p in sorted(visited)],
    'static_edges': edges, 'unresolved_references': unresolved,
    'feature_processor_binary_inputs': [info(p) for p in runtime_binary],
    'default_grid_and_whitebox_uv_inventory': [info(p) for p in sorted(historical)],
    'limits': 'Static conservative reachability including disabled pass requests; dynamic builders/registries/code loads require real AP/Editor evidence. No source binary or product readiness is certified.'})

if '--local-only' in sys.argv:
    print(json.dumps({'mode': 'local static inspection only', 'sources': len(visited), 'unresolved': len(unresolved)}))
    sys.exit(0)

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, msg, headers, newurl):
        return None

opener = urllib.request.build_opener(NoRedirect)
def request(url, method='GET'):
    if not url.startswith(('https://api.github.com/repos/o3de/o3de/', 'https://media.githubusercontent.com/media/o3de/o3de/')):
        raise ValueError('Unapproved audit host/path')
    req = urllib.request.Request(url, method=method, headers={'User-Agent': 'JE-static-assets-audit', 'Accept': 'application/vnd.github+json'})
    try:
        response = opener.open(req, timeout=30)
    except urllib.error.HTTPError as error:
        response = error
    headers = {key: response.headers.get(key) for key in ['Content-Type','Content-Length','Location','ETag']}
    body = response.read() if method == 'GET' and url.startswith('https://api.github.com/') else b''
    return {'url': url, 'method': method, 'status': response.code, 'headers': headers,
            'authorization': False, 'automatic_redirects': False}, body

receipt, body = request('https://api.github.com/repos/o3de/o3de/commits/development')
save('github-development-metadata.json', {'receipt': receipt, 'data': json.loads(body)})
commit = json.loads(body)['sha']
selected = ['Code/Editor/res/o3de_editor.ico', 'Code/LauncherUnified/Resources/GameSDK.ico',
            'Templates/ScriptOnlyProject/Template/Resources/Splash.bmp', 'Assets/Engine/Fonts/Vera.ttf',
            'Gems/Atom/Feature/Common/Assets/Textures/Default/default_iblglobalcm.dds']
destinations = []
for relative in selected:
    url = 'https://api.github.com/repos/o3de/o3de/contents/' + urllib.parse.quote(relative) + '?ref=' + commit
    metadata_receipt, metadata_body = request(url)
    data = json.loads(metadata_body)
    pointer = base64.b64decode(data.get('content', '')) if data.get('encoding') == 'base64' else b''
    payload_url = 'https://media.githubusercontent.com/media/o3de/o3de/' + commit + '/' + urllib.parse.quote(relative)
    head, _ = request(payload_url, 'HEAD')
    destinations.append({'path': relative, 'commit': commit, 'metadata_receipt': metadata_receipt,
                         'git_blob_sha1': data.get('sha'), 'pointer_matches_local_bytes': pointer == (ROOT / relative).read_bytes(),
                         'payload_head': head, 'metadata_source': url, 'payload_not_downloaded': True})
    save('github-payload-head-audit.json', destinations)
print(json.dumps({'resources': len(resources), 'qrc_inputs': len(qrc_inputs), 'closure_sources': len(visited),
                  'unresolved': len(unresolved), 'font_original_matches': font_duplicates,
                  'github_head': [(item['path'], item['payload_head']['status']) for item in destinations]}, indent=2))

import ast, hashlib, json, pathlib
repo=pathlib.Path(r'F:\JericoEngine\JericoEngine')
manifest_path=repo/'build/n0-validation/local-packages/source-freeze-editor-fixture-publication-parent.json'
manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
checks=[]
for item in manifest:
    p=repo/item['path']; data=p.read_bytes() if p.is_file() else b''; actual=hashlib.sha256(data).hexdigest().lower()
    checks.append({'path':item['path'],'exists':p.is_file(),'bytes_expected':item['bytes'],'bytes_actual':len(data),'sha256_expected':item['sha256'].lower(),'sha256_actual':actual,'match':p.is_file() and len(data)==item['bytes'] and actual==item['sha256'].lower()})
author_path=repo/'Projects/JEEditorFixture/Editor/Scripts/author_room.py'
tree=ast.parse(author_path.read_text(encoding='utf-8'),filename=str(author_path))
ast_summary={'parse_ok':True,'functions':[n.name for n in tree.body if isinstance(n,(ast.FunctionDef,ast.AsyncFunctionDef))],'imports':sorted({a.name for n in ast.walk(tree) if isinstance(n,ast.Import) for a in n.names}|{n.module for n in ast.walk(tree) if isinstance(n,ast.ImportFrom) and n.module})}
json_checks=[]
for item in manifest:
    p=repo/item['path']
    if p.suffix.lower() in {'.json','.setreg','.prefab','.material'}:
        try: value=json.loads(p.read_text(encoding='utf-8')); json_checks.append({'path':item['path'],'valid':True,'root_type':type(value).__name__})
        except Exception as exc: json_checks.append({'path':item['path'],'valid':False,'error':f'{type(exc).__name__}: {exc}'})
result={'manifest_path':str(manifest_path),'manifest_sha256':hashlib.sha256(manifest_path.read_bytes()).hexdigest().upper(),'manifest_file_count':len(manifest),'files_matching':sum(x['match'] for x in checks),'file_mismatches':[x for x in checks if not x['match']],'ast':ast_summary,'json_checks':json_checks,'all_pass':len(manifest)==22 and all(x['match'] for x in checks) and ast_summary['parse_ok'] and len(json_checks)==6 and all(x['valid'] for x in json_checks)}
print(json.dumps(result,indent=2))
if not result['all_pass']: raise SystemExit(1)

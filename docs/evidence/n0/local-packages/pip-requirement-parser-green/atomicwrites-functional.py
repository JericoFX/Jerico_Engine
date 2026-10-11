import hashlib, importlib.metadata as md, json, pathlib, sys
from atomicwrites import atomic_write
root=pathlib.Path(sys.argv[1]); root.mkdir(parents=True,exist_ok=True)
results=[]
def check(name, ok, detail):
    results.append({'case':name,'pass':bool(ok),'detail':detail})
# overwrite=True replaces old bytes with fully written new content.
p=root/'overwrite-true.txt'; p.write_bytes(b'old-value')
with atomic_write(str(p),overwrite=True,encoding='utf-8') as f: f.write('new-value')
check('overwrite_true_replaces',p.read_bytes()==b'new-value',{'observed':p.read_bytes().decode('utf-8','replace')})
# overwrite=False should raise when target already exists and preserve it.
p=root/'overwrite-false.txt'; p.write_bytes(b'preserve-this')
try:
    with atomic_write(str(p),overwrite=False,encoding='utf-8') as f: f.write('should-not-commit')
except FileExistsError as e:
    outcome='FileExistsError'
except Exception as e:
    outcome=f'{type(e).__name__}: {e}'
else: outcome='NO_EXCEPTION'
check('overwrite_false_rejects_existing',outcome=='FileExistsError' and p.read_bytes()==b'preserve-this',{'exception':outcome,'contents':p.read_bytes().decode('utf-8','replace')})
# An exception while writing must abort and preserve pre-existing target.
p=root/'exception-abort.txt'; p.write_bytes(b'original')
try:
    with atomic_write(str(p),overwrite=True,encoding='utf-8') as f:
        f.write('partial-update')
        raise RuntimeError('intentional-context-abort')
except RuntimeError as e:
    exception=f'{type(e).__name__}: {e}'
else: exception='NO_EXCEPTION'
check('exception_preserves_original',exception=='RuntimeError: intentional-context-abort' and p.read_bytes()==b'original',{'exception':exception,'contents':p.read_bytes().decode('utf-8','replace')})
files=sorted(x.name for x in root.iterdir() if x.is_file())
expected=['exception-abort.txt','overwrite-false.txt','overwrite-true.txt']
check('no_temporary_files_remain',files==expected,{'files':files,'expected':expected})
result={'atomicwrites_version':md.version('atomicwrites'),'module_path':__import__('atomicwrites').__file__,'scratch_root':str(root),'checks':results,'all_pass':all(x['pass'] for x in results)}
print(json.dumps(result,indent=2))
if not result['all_pass']: raise SystemExit(1)

import hashlib, json, pathlib, re, sys
from pip._internal.network.session import PipSession
from pip._internal.req.req_file import parse_requirements
from packaging.requirements import Requirement
root=pathlib.Path(r'F:\JericoEngine\JericoEngine')
req=root/'python/requirements.txt'
build=req.with_name('requirements-build.txt')
expected='d2891893d22b227c5d85bd4f1d2ebcca63a67c767be64a79cf7121100ddc6a6e'
raw=req.read_bytes(); sha=hashlib.sha256(raw).hexdigest()
if sha != expected: raise SystemExit(f'requirements sha mismatch: {sha}')
text=raw.decode('utf-8')
lex=[]; current=None
for n,line in enumerate(text.splitlines(),1):
    m=re.match(r'\s*([A-Za-z0-9_.-]+)==([^\s\\]+)',line)
    if m:
        current={'line':n,'name':m.group(1).lower().replace('_','-'), 'version':m.group(2), 'hashes':[]}
        lex.append(current)
    elif current:
        hm=re.search(r'--hash=sha256:([0-9a-fA-F]{64})',line)
        if hm: current['hashes'].append(hm.group(1).lower())
        elif line.strip() and not line.lstrip().startswith('#') and not line.rstrip().endswith('\\'):
            current=None

def parse(path):
    rows=[]
    for item in parse_requirements(str(path),session=PipSession()):
        text=getattr(item,'requirement',None)
        if text:
            r=Requirement(text)
            rows.append({'name':r.name.lower().replace('_','-'),'specifier':str(r.specifier),'comes_from':str(getattr(item,'comes_from','')),'hashes':list((getattr(item,'options',None) or {}).get('hashes',{}).get('sha256',[]))})
    return rows
rows=parse(req); buildrows=parse(build)
byname={r['name']:r for r in rows}
checks=[]
for item in lex:
    got=byname.get(item['name'])
    checks.append({'name':item['name'],'line':item['line'],'expected_version':item['version'],'parsed_specifier':got['specifier'] if got else None,'expected_hashes':item['hashes'],'parsed_hashes':got['hashes'] if got else None,'pass':bool(got and got['specifier']==f"=={item['version']}" and item['hashes'] and sorted(got['hashes'])==sorted(item['hashes']))})
result={'pip_version':__import__('pip').__version__,'parser':'pip._internal.req.req_file.parse_requirements + packaging.Requirement; local files only','requirements_sha256':sha,'requirements_build_sha256':hashlib.sha256(build.read_bytes()).hexdigest(),'lexical_count':len(lex),'parsed_count':len(rows),'build_rows':buildrows,'pin_checks':checks,'all_pins_pass':len(lex)==43 and len(rows)==43 and len(byname)==43 and all(x['pass'] for x in checks),'wheel_unique':len(buildrows)==1 and buildrows[0]['name']=='wheel' and buildrows[0]['specifier']=='==0.38.4'}
print(json.dumps(result,indent=2))
if not result['all_pins_pass'] or not result['wheel_unique']: raise SystemExit(1)

import importlib.metadata as md, importlib.util, json, pathlib, re, sys
from packaging.utils import canonicalize_name
from packaging.version import Version
root=pathlib.Path(r'F:\JericoEngine\JericoEngine')
req=root/'python/requirements.txt'; build=root/'python/requirements-build.txt'
pins=[]
for n,line in enumerate(req.read_text(encoding='utf-8').splitlines(),1):
    m=re.match(r'\s*([A-Za-z0-9_.-]+)==([^\s\\]+)',line)
    if m: pins.append({'line':n,'name':canonicalize_name(m.group(1)),'version':m.group(2)})
versions={canonicalize_name(d.metadata['Name']):d.version for d in md.distributions() if d.metadata.get('Name')}
checks=[]
for p in pins:
    actual=versions.get(p['name'])
    try: semantic=(actual is not None and Version(actual)==Version(p['version']))
    except Exception: semantic=False
    checks.append({'line':p['line'],'name':p['name'],'pinned_version':p['version'],'metadata_version':actual,'pep440_equal':semantic,'pass':semantic})
buildpins=[]
for line in build.read_text(encoding='utf-8').splitlines():
    m=re.match(r'\s*([A-Za-z0-9_.-]+)==([^\s\\]+)',line)
    if m: buildpins.append({'name':canonicalize_name(m.group(1)),'version':m.group(2)})
aws=[]
for n in ('boto3','botocore','s3transfer'):
    try: dist=md.version(n)
    except md.PackageNotFoundError: dist=None
    module=importlib.util.find_spec(n)
    aws.append({'name':n,'distribution_version':dist,'module_spec':bool(module),'absent':dist is None and module is None})
result={'python_version':sys.version,'python_executable':sys.executable,'pip_version':md.version('pip'),'requirements_lexical_count':len(pins),'requirements_pep440_matching_count':sum(x['pass'] for x in checks),'requirement_pins':checks,'build_requirements':{'pins':buildpins,'installed_version':versions.get('wheel'),'unique_wheel_0384':len(buildpins)==1 and buildpins[0]=={'name':'wheel','version':'0.38.4'} and versions.get('wheel')=='0.38.4'},'aws_absence':aws,'all_pass':len(pins)==43 and all(x['pass'] for x in checks) and len(buildpins)==1 and buildpins[0]=={'name':'wheel','version':'0.38.4'} and versions.get('wheel')=='0.38.4' and all(x['absent'] for x in aws)}
print(json.dumps(result,indent=2))
if not result['all_pass']: raise SystemExit(1)

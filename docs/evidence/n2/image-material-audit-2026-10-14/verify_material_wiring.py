#!/usr/bin/env python3
"""Verify scalar property compatibility/wiring; execute the actual Lua functors.

No material compiler, Shader Resource Group upload, Editor or GPU execution.
Lua contexts below are test-only seams for the native editor API.
"""
import argparse
import ctypes
import json
from pathlib import Path
import re
import sys

def strip_comments(text):
    return re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/',lambda m:m.group(0) if m.group(0).startswith('"') else '',text,flags=re.S)

def json_with_comments(path):
    return json.loads(strip_comments(path.read_text()))

def check_schema(repo,preimages):
    base=Path('Gems/Atom/Feature/Common/Assets/Materials/Types')
    fields={
        'Roughness':{'textureMapChannel':'m_roughnessMapChannel'},
        'Metallic':{'textureMapChannel':'m_metallicMapChannel'},
        'Occlusion':{'diffuseTextureMapChannel':'m_diffuseOcclusionMapChannel','specularTextureMapChannel':'m_specularOcclusionMapChannel'},
    }
    for group,selectors in fields.items():
        rel=base/'MaterialInputs'/(group+'PropertyGroup.json')
        current=json_with_comments(repo/rel)
        original=json_with_comments(preimages/rel)
        properties={p['name']:p for p in current['properties']}
        assert len(properties)==len(current['properties']),str(rel)+' duplicate property'
        for name,srg in selectors.items():
            item=properties[name]
            assert item['type']=='Enum' and item['enumValues']==['Red','Green','Blue','Alpha'] and item['defaultValue']=='Red'
            assert item['connection']=={'type':'ShaderInput','name':srg}
        for prop in original['properties']:
            assert properties[prop['name']]==prop,str(rel)+' existing property changed'
        if group=='Occlusion':
            assert all(any(selector in f['args']['dependentProperties'] for f in current['functors']) for selector in selectors)
    for name in ['BasePBR','StandardPBR','EnhancedPBR','Skin','Eye','StandardMultilayerPBR']:
        rel=base/(name+'.materialtype')
        current=json_with_comments(repo/rel)
        original=json_with_comments(preimages/rel)
        assert current['version']==original['version']+1
        prior=original.get('versionUpdates',[])
        assert current['versionUpdates'][:-1]==prior
        assert current['versionUpdates'][-1]=={'toVersion':current['version'],'actions':[]}
        for key,value in original.items():
            if key not in ['version','versionUpdates']:assert current[key]==value,str(rel)+' unrelated metadata changed'
    print('PASS schema: selectors, legacy properties/defaults, six additive version updates')

def check_shader_wiring(repo):
    base=repo/'Gems/Atom/Feature/Common/Assets/Shaders/Materials'
    calls=0
    for name in ['BasePBR','StandardPBR','EnhancedPBR','Skin','Eye','StandardMultilayerPBR']:
        text=strip_comments((base/name/(name+'_SurfaceEval.azsli')).read_text())
        for match in re.finditer(r'Get(?:Roughness|Metallic|Occlusion)Input\(',text):
            pos=match.end();start=pos;depth=1
            while depth:
                if text[pos]=='(':depth+=1
                elif text[pos]==')':depth-=1
                pos+=1
            arguments=text[start:pos-1]
            assert 'MapChannel' in arguments.rsplit(',',1)[-1],name+' omits channel propagation'
            calls+=1
    multi=(base/'StandardMultilayerPBR/StandardMultilayerPBR_SurfaceEval.azsli').read_text()
    for channel in ['roughness','metallic','diffuseOcclusion','specularOcclusion']:
        assert re.search(r'm_'+channel+r'MapChannel\s*=\s*srgLayerPrefix##m_'+channel+r'MapChannel',multi),channel+' multilayer transfer missing'
    print('PASS shader source wiring:',calls,'scalar callsites and four multilayer channel copies')

def check_lua(repo):
    lib=ctypes.CDLL('liblua5.4.so.0')
    lib.luaL_newstate.restype=ctypes.c_void_p
    lib.luaL_openlibs.argtypes=[ctypes.c_void_p]
    lib.luaL_loadstring.argtypes=[ctypes.c_void_p,ctypes.c_char_p]
    lib.luaL_loadstring.restype=ctypes.c_int
    lib.lua_pcallk.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_longlong,ctypes.c_void_p]
    lib.lua_pcallk.restype=ctypes.c_int
    lib.lua_tolstring.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_void_p]
    lib.lua_tolstring.restype=ctypes.c_char_p
    lib.lua_close.argtypes=[ctypes.c_void_p]
    for name in ['Roughness','Metallic']:
        path=(repo/'Gems/Atom/Feature/Common/Assets/Materials/Types'/('StandardPBR_'+name+'.lua')).resolve()
        script='dofile('+json.dumps(str(path))+')\n'+r'''
MaterialPropertyVisibility_Enabled=1
MaterialPropertyVisibility_Disabled=2
MaterialPropertyVisibility_Hidden=3
for _,case in ipairs({{has=false,use=true,want=3},{has=true,use=false,want=2},{has=true,use=true,want=1}}) do
    local visibility={}
    local enabled=nil
    local context={}
    function context:GetMaterialPropertyValue_Image(_) if case.has then return {} end return nil end
    function context:GetMaterialPropertyValue_bool(_) return case.use end
    function context:SetMaterialPropertyVisibility(name,value) visibility[name]=value end
    function context:SetShaderOptionValue_bool(_,value) enabled=value end
    ProcessEditor(context)
    assert(visibility['textureMapChannel']==case.want,'channel visibility is wrong')
    Process(context)
    assert(enabled==(case.has and case.use),'texture enable option is wrong')
end
'''
        state=lib.luaL_newstate()
        try:
            lib.luaL_openlibs(state)
            status=lib.luaL_loadstring(state,script.encode())
            if status==0:status=lib.lua_pcallk(state,0,0,0,0,None)
            if status:raise AssertionError(lib.lua_tolstring(state,-1,None).decode())
        finally:lib.lua_close(state)
    print('PASS actual Lua functors: missing/disabled/enabled image channel visibility')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--preimages',type=Path,required=True)
    args=parser.parse_args()
    check_schema(args.repo,args.preimages)
    check_shader_wiring(args.repo)
    check_lua(args.repo)
    return 0

if __name__=='__main__':sys.exit(main())

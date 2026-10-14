#!/usr/bin/env python3
"""Compile the actual scalar AZSL bodies as C++ with test-only sampler/type shims.

This checks channel arithmetic and branch wiring only. It does not run AZSL,
GPU sampling, material compilation, the Editor, Asset Processor or a cooker.
No external packages, source rewriting or product runtime dependency.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

CPP = r'''
#include <cmath>
#include <cstdio>
#include <limits>
using real = float;
using uint = unsigned int;
struct float2 { float x, y; };
struct float4 {
    float r, g, b, a;
    float2 xy, zw;
    float4(float x, float y, float z, float w): r(x),g(y),b(z),a(w),xy{x,y},zw{z,w} {}
    float operator[](uint c) const { return c == 0 ? r : c == 1 ? g : c == 2 ? b : a; }
};
struct sampler {};
struct Texture2D {
    float4 value;
    float4 Sample(sampler, float2) const { return value; }
    float4 SampleGrad(sampler, float2, float2, float2) const { return value; }
    float4 SampleLevel(sampler, float2, int) const { return value; }
};
bool overrides = false;
bool OverrideRoughnessEnabled() { return overrides; }
float GetRoughnessOverride() { return .37f; }
bool OverrideMetallicEnabled() { return overrides; }
float GetMetallicOverride() { return .73f; }
float lerp(float a, float b, float t) { return a+(b-a)*t; }
@INCLUDES@

float rough(Texture2D t, uint c, bool grad, bool use=true, float lower=0, float upper=1) {
    return GetRoughnessInput(t, {}, {0,0}, .45f, lower, upper, use, {0,0,0,0}, grad
#ifdef JE_TEST_ROUGHNESS_CHANNEL
        , c
#endif
    );
}
float metal(Texture2D t, uint c, bool grad, bool use=true) {
    return GetMetallicInput(t, {}, {0,0}, .35f, use, {0,0,0,0}, grad
#ifdef JE_TEST_METALLIC_CHANNEL
        , c
#endif
    );
}
float ao(Texture2D t, uint c, bool grad, bool use=true, float strength=1) {
    return GetOcclusionInput(t, {}, {0,0}, strength, use, {0,0,0,0}, grad
#ifdef JE_TEST_OCCLUSION_CHANNEL
        , c
#endif
    );
}
int failures=0;
void check(const char* name, float got, float expected) {
    if (!std::isfinite(got) || std::fabs(got-expected)>1e-6f) {
        std::printf("FAIL %s got=%.8f expected=%.8f\n", name, got, expected); ++failures;
    }
}
int main() {
    Texture2D packed{{.2f,.6f,.9f,.4f}};
    Texture2D separateRough{{.6f,0,0,1}}, separateMetal{{.9f,0,0,1}}, separateAo{{.2f,0,0,1}};
    for (bool grad: {false,true}) {
        check("packed-ORM-vs-separated-roughness", rough(packed,1,grad),rough(separateRough,0,grad));
        check("packed-ORM-vs-separated-metallic", metal(packed,2,grad),metal(separateMetal,0,grad));
        check("packed-ORM-vs-separated-AO", ao(packed,0,grad),ao(separateAo,0,grad));
        for (uint c=0;c<4;++c) {
            check("roughness-selected-channel",rough(packed,c,grad),packed.value[c]);
            check("metallic-selected-channel",metal(packed,c,grad),packed.value[c]);
            check("AO-selected-channel",ao(packed,c,grad),packed.value[c]);
        }
        check("roughness-invalid-channel-red",rough(packed,99,grad),.2f);
        check("metallic-invalid-channel-red",metal(packed,99,grad),.2f);
        check("AO-invalid-channel-red",ao(packed,99,grad),.2f);
        check("roughness-bounds",rough(packed,1,grad,true,.1f,.8f),.52f);
        check("AO-strength",ao(packed,3,grad,true,2),.16f);
        check("roughness-no-texture-factor",rough(packed,2,grad,false),.45f);
        check("metallic-no-texture-factor",metal(packed,2,grad,false),.35f);
        check("AO-no-texture-unoccluded",ao(packed,2,grad,false),1.f);
    }
    overrides=true;
    check("roughness-debug-override",rough(packed,1,false),.37f);
    check("metallic-debug-override",metal(packed,2,false),.73f);
    // Existing callers omit both optional parameters; this must retain red.
    overrides=false;
    check("legacy-roughness-red",GetRoughnessInput(packed,{}, {0,0},.45f,0.f,1.f,true),.2f);
    check("legacy-metallic-red",GetMetallicInput(packed,{}, {0,0},.35f,true),.2f);
    check("legacy-AO-red",GetOcclusionInput(packed,{}, {0,0},1.f,true),.2f);
    std::printf("scalar shader CPU arithmetic failures=%d\n",failures);
    return failures ? 1 : 0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    inputs=args.repo/'Gems/Atom/Feature/Common/Assets/Shaders/Materials/MaterialInputs'
    include=[]
    defines=[]
    source_hashes={}
    for name in ['Roughness','Metallic','Occlusion']:
        path=(inputs/(name+'Input.azsli')).resolve()
        body=path.read_text()
        source_hashes[str(path.relative_to(args.repo.resolve()))]=hashlib.sha256(path.read_bytes()).hexdigest()
        include.append('#include "'+str(path)+'"')
        # Adapt only the call arity, never the production function body. The
        # exact same expectations run before and after the optional argument.
        signature=re.search(r'real Get'+name+r'Input\((.*?)\)\s*\{',body,re.S)
        if signature and 'textureChannel' in signature.group(1):
            defines.append('-DJE_TEST_'+name.upper()+'_CHANNEL')
    stub=args.output/'include/Atom/Features/Debug.azsli'
    stub.parent.mkdir(parents=True)
    stub.write_text('// Test-only debug implementation provided in CPU harness.\n')
    cpp=args.output/'scalar_shader_cpu.cpp'
    cpp.write_text('#include <initializer_list>\n'+CPP.replace('@INCLUDES@','\n'.join(include)))
    outcomes=[]
    for mode,extra in [('pixel',[]),('compute',['-DCS_SAMPLERS'])]:
        binary=args.output/('scalar-'+mode)
        cmd=['g++','-std=c++17','-Wall','-Wextra','-Wno-unused-parameter','-Werror',*defines,*extra,'-I'+str(args.output/'include'),str(cpp),'-o',str(binary)]
        build=subprocess.run(cmd,capture_output=True,text=True)
        (args.output/(mode+'-build.log')).write_text(build.stdout+build.stderr)
        run=subprocess.run([str(binary)],capture_output=True,text=True) if build.returncode==0 else None
        if run:
            (args.output/(mode+'-run.log')).write_text(run.stdout+run.stderr)
        outcome={'mode':mode,'command':cmd,'buildExit':build.returncode,'runExit':run.returncode if run else None,'output':run.stdout+run.stderr if run else build.stdout+build.stderr}
        outcomes.append(outcome)
        print(mode, 'build',build.returncode,'run',run.returncode if run else 'NOT_RUN')
        print(outcome['output'],end='')
    result={'scope':'Actual AZSL scalar function bodies compiled as C++; test-only texture/type/debug shims. No GPU/AZSL/Editor/AP/cook acceptance.','sourceSha256':source_hashes,'outcomes':outcomes,'passed':all(x['buildExit']==0 and x['runExit']==0 for x in outcomes)}
    (args.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed'] else 1

if __name__=='__main__':
    sys.exit(main())

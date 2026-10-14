#!/usr/bin/env python3
"""Execute the exact EditorTextureSetting::InitFromPath definition with C++ test seams.

Only preset-selection control flow is exercised. Image decoding, real .assetinfo
serialization, Qt widgets, AP/cook and end-to-end persistence are not executed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

CPP = r'''
#include <map>
#include <string>
#include <memory>
#include <cstdio>
#include <cstdlib>
#define AZ_Warning(...) ((void)0)
#define AZ_Assert(cond,...) do { if (!(cond)) std::abort(); } while(false)
#define AZ_Error(...) ((void)0)
namespace AZStd { using string=std::string; }
struct PresetName {
    std::string name;
    const char* GetCStr() const { return name.c_str(); }
};
struct PresetSettings {};
struct Image {};
using IImageObjectPtr=std::shared_ptr<Image>;
Image* LoadImageFromFile(const std::string&) { return new Image; }
namespace AzFramework::StringFunc::Path {
    void GetFullFileName(const char* path,std::string& out) { out=path; }
}
struct Setting {
    PresetName m_preset;
    int reduction;
    bool mipmaps;
    void ApplyPreset(PresetName p) { m_preset=p; reduction=0; mipmaps=true; }
};
std::map<std::string,Setting> sourceSettings;
struct TextureSettings {
    static auto GetMultiplatformTextureSetting(const std::string&,bool& generated) {
        generated=false;return sourceSettings;
    }
};
struct BuilderSettingManager {
    static BuilderSettingManager* Instance() { static BuilderSettingManager x;return &x; }
    const PresetSettings* GetPreset(PresetName p) {
        static PresetSettings x;return p.name=="Normals" || p.name=="Albedo" ? &x : nullptr;
    }
    PresetName GetSuggestedPreset(const std::string&) { return {"Albedo"}; }
};
namespace ImageProcessingAtomEditor {
struct EditorTextureSetting {
    std::string m_fullPath,m_textureName;
    IImageObjectPtr m_img;
    std::map<std::string,Setting> m_settingsMap;
    void InitFromPath(const AZStd::string& texturePath);
};
@DEFINITION@
}
int main() {
    int failures=0;
    for (const char* filename: {"vacaciones.png","contradictorio_diff.png"}) {
        for (const char* preset: {"Normals","unavailable-explicit-preset"}) {
            sourceSettings={{"pc",{{preset},3,false}},{"linux",{{preset},2,false}}};
            ImageProcessingAtomEditor::EditorTextureSetting setting;
            setting.InitFromPath(filename);
            for (const auto& [platform,expected]: sourceSettings) {
                const auto& actual=setting.m_settingsMap.at(platform);
                if (actual.m_preset.name!=expected.m_preset.name || actual.reduction!=expected.reduction || actual.mipmaps!=expected.mipmaps) {
                    std::printf("FAIL explicit-preset-preservation %s %s preset=%s reduction=%d mipmaps=%d\n",filename,platform.c_str(),actual.m_preset.GetCStr(),actual.reduction,actual.mipmaps);
                    ++failures;
                }
            }
        }
    }
    std::printf("preset-selection CPU control flow failures=%d\n",failures);
    return failures ? 1 : 0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    path=args.repo/'Gems/Atom/Asset/ImageProcessingAtom/Code/Source/Editor/EditorCommon.cpp'
    text=path.read_text()
    token='void EditorTextureSetting::InitFromPath('
    start=text.index(token)
    opening=text.index('{',start)
    depth=1
    end=opening+1
    # This existing definition has no braces in its string/comment literals.
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    definition=text[start:end]
    cpp=args.output/'preset_cpu.cpp'
    cpp.write_text(CPP.replace('@DEFINITION@',definition))
    binary=args.output/'preset-control-flow'
    command=['g++','-std=c++17','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)]
    build=subprocess.run(command,text=True,capture_output=True)
    (args.output/'build.log').write_text(build.stdout+build.stderr)
    run=subprocess.run([str(binary)],text=True,capture_output=True) if build.returncode==0 else None
    if run:(args.output/'run.log').write_text(run.stdout+run.stderr)
    result={'scope':'Exact production InitFromPath definition; test-only decoding/settings/configuration seams. No real image import or sidecar/Qt/AP/cook execution.','sourceSha256':hashlib.sha256(path.read_bytes()).hexdigest(),'definitionSha256':hashlib.sha256(definition.encode()).hexdigest(),'command':command,'buildExit':build.returncode,'runExit':run.returncode if run else None,'passed':build.returncode==0 and run is not None and run.returncode==0}
    (args.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('build',build.returncode,'run',run.returncode if run else 'NOT_RUN')
    print(run.stdout+run.stderr if run else build.stdout+build.stderr,end='')
    return 0 if result['passed'] else 1

if __name__=='__main__':sys.exit(main())

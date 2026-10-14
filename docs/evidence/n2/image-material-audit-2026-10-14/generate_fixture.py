#!/usr/bin/env python3
"""Generate owned synthetic test inputs in a NEW directory; never edits existing sources.

External test tooling only. No import, metadata serializer, cooker or product VM.
The Editor must write real .assetinfo files through its existing texture settings.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib

def png_rgba(pixels, width=4, height=4):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+pixels[y*width*4:(y+1)*width*4] for y in range(height))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')

def ktx2_rgba8(pixels, width=4, height=4):
    # KTX 2.0: uncompressed VK_FORMAT_R8G8B8A8_UNORM (37), one level,
    # RGBSDA/BT709/LINEAR basic DFD v2, straight alpha, 8-bit UNORM samples.
    block=struct.pack('<IHH4B4B8B',0,2,88,1,1,1,0,0,0,0,0,4,0,0,0,0,0,0,0)
    for offset,channel in [(0,0),(8,1),(16,2),(24,15)]:
        block+=struct.pack('<HBB4BII',offset,7,channel,0,0,0,0,0,255)
    dfd=struct.pack('<I',92)+block
    header=b'\xabKTX 20\xbb\r\n\x1a\n'+struct.pack('<13I2Q',37,1,width,height,0,0,1,1,0,104,len(dfd),0,0,0,0)
    level=struct.pack('<3Q',104+len(dfd),len(pixels),len(pixels))
    assert len(header)==80 and len(dfd)==92
    return header+level+dfd+pixels

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    type_file=args.repo/'Gems/Atom/Feature/Common/Assets/Materials/Types/StandardPBR.materialtype'
    version=int(re.search(r'"version"\s*:\s*(\d+)',type_file.read_text()).group(1))
    pixels={
        'vacaciones.png':(128,64,32,255),
        'tornillo.png':(128,204,230,255),
        'cuaderno.png':(128,51,230,255),
        'caja.png':(51,153,230,255),
        'papel.png':(51,51,51,255),
        'boton.png':(153,153,153,255),
        'taza.png':(230,230,230,255),
    }
    for name,rgba in pixels.items():
        (args.output/name).write_bytes(png_rgba(bytes(rgba)*16))
    (args.output/'archivo.ktx2').write_bytes(ktx2_rgba8(bytes(pixels['caja.png'])*16))
    common={
        'baseColor.textureMap':'vacaciones.png','baseColor.useTexture':True,
        'normal.textureMap':'tornillo.png','normal.useTexture':True,
        'normal.flipX':False,'normal.flipY':False,
        'roughness.useTexture':True,'roughness.lowerBound':0.0,'roughness.upperBound':1.0,
        'metallic.useTexture':True,'occlusion.diffuseUseTexture':True,'occlusion.diffuseFactor':1.0,
        'occlusion.specularUseTexture':False,
    }
    cases={
        'JePacked.material':{'roughness.textureMap':'caja.png','roughness.textureMapChannel':'Green','metallic.textureMap':'caja.png','metallic.textureMapChannel':'Blue','occlusion.diffuseTextureMap':'caja.png','occlusion.diffuseTextureMapChannel':'Red'},
        'JeSeparate.material':{'roughness.textureMap':'boton.png','roughness.textureMapChannel':'Red','metallic.textureMap':'taza.png','metallic.textureMapChannel':'Red','occlusion.diffuseTextureMap':'papel.png','occlusion.diffuseTextureMapChannel':'Red'},
        'JeKtx2.material':{'roughness.textureMap':'archivo.ktx2','roughness.textureMapChannel':'Green','metallic.textureMap':'archivo.ktx2','metallic.textureMapChannel':'Blue','occlusion.diffuseTextureMap':'archivo.ktx2','occlusion.diffuseTextureMapChannel':'Red'},
    }
    for name,values in cases.items():
        material={'materialType':'@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR.materialtype','materialTypeVersion':version,'propertyValues':dict(common,**values)}
        (args.output/name).write_text(json.dumps(material,indent=2)+'\n')
    manifest={
        'scope':'Fixture bytes/material source assignments generated and locally checked; native import/metadata/cook/reimport/save-reopen NOT_RUN.',
        'ktxSpecification':'https://github.khronos.org/KTX-Specification/ktxspec.v2.html',
        'ktxLimit':'Local structural check only; no Khronos validator/decoder. Single level does not test mip generation or Basis supercompression.',
        'manualTexturePresets':{'vacaciones.png':'Albedo','tornillo.png':'Normals','cuaderno.png':'Normals','caja.png':'ReferenceImage_Linear','papel.png':'ReferenceImage_Linear','boton.png':'ReferenceImage_Linear','taza.png':'ReferenceImage_Linear'},
        'presetLimit':'ReferenceImage_Linear preserves RGBA, no mip chain; full mips/compression gate stays open.',
        'expectedOrmLinear':{'AO':51/255,'Roughness':153/255,'Metallic':230/255},
        'sourceIdentity':'First import obtains UUIDs/subIDs from the existing AP. Record them before reimport; this generator never invents UUIDs.',
        'files':{p.name:{'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(args.output.iterdir())},
    }
    (args.output/'fixture-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Generated',len(manifest['files']),'test inputs in',args.output)

if __name__=='__main__':main()

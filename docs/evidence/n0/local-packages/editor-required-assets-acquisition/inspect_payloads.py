"""Read asset containers/font name tables as data only; never render, import or execute them."""
import collections
import hashlib
import json
import struct
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path('F:/JericoEngine/JericoEngine')
OUT = ROOT / 'build/n0-validation/local-packages/editor-required-assets-acquisition'
SNAPSHOT = Path('F:/JericoEngine/Dependencies/assets/n0-original-editor')
MANIFEST = ROOT / 'build/n0-validation/local-packages/editor-required-assets-audit/acquisition-review-manifest.json'
PIN = '7982d0db8511004f342af03f5e73b2dab510974b7be37508a0c640bfc0dd95a4'

def unpack(format, data, offset=0):
    if offset < 0 or offset+struct.calcsize(format) > len(data):
        raise ValueError('Container field exceeds file bounds')
    return struct.unpack_from(format, data, offset)

def font(data):
    signature, tables = unpack('>IH', data)
    if signature not in (0x00010000, 0x4f54544f):
        raise ValueError('Unexpected sfnt signature')
    found = {}
    for index in range(tables):
        tag, checksum, offset, length = unpack('>4sIII', data, 12+16*index)
        if offset+length > len(data):
            raise ValueError('Font table exceeds file bounds')
        found[tag] = (offset, length)
    name_offset, name_length = found[b'name']
    name_format, count, strings = unpack('>HHH', data, name_offset)
    names = []
    for index in range(count):
        platform, encoding, language, name_id, length, offset = unpack('>HHHHHH', data, name_offset+6+12*index)
        start = name_offset+strings+offset
        if start+length > name_offset+name_length:
            raise ValueError('Font name exceeds name-table bounds')
        if name_id in (0,1,2,3,4,5,6,7,8,9,10,11,13,14):
            raw = data[start:start+length]
            value = raw.decode('utf-16-be' if platform in (0,3) else 'mac_roman', errors='strict')
            names.append({'platform': platform, 'encoding': encoding, 'language': language, 'name_id': name_id, 'text': value})
    return {'kind': 'sfnt font', 'tables': tables, 'name_format': name_format, 'names_and_full_notices': names}

def inspect(data, extension):
    if extension == '.ttf':
        return font(data)
    if extension == '.font':
        root = ET.fromstring(data)
        return {'kind': 'font XML descriptor', 'root': root.tag,
                'font_references': [row.attrib for row in root.iter('font')]}
    if data[:8] == b'\x89PNG\r\n\x1a\n':
        length, tag, width, height, bits, color, compression, filtering, interlace = unpack('>I4sIIBBBBB', data, 8)
        if tag != b'IHDR' or length != 13 or not width or not height:
            raise ValueError('Invalid PNG IHDR')
        return {'kind': 'PNG', 'width': width, 'height': height, 'bits': bits, 'color_type': color,
                'compression': compression, 'filter': filtering, 'interlace': interlace}
    if extension == '.ico':
        reserved, kind, count = unpack('<HHH', data)
        if reserved or kind != 1 or not count:
            raise ValueError('Invalid ICO header')
        images = []
        for index in range(count):
            width, height, colors, reserved, planes, bits, size, offset = unpack('<BBBBHHII', data, 6+16*index)
            if offset+size > len(data):
                raise ValueError('ICO image exceeds container')
            images.append({'width': width or 256, 'height': height or 256, 'bits': bits, 'bytes': size, 'offset': offset,
                           'encoding': 'PNG' if data[offset:offset+8] == b'\x89PNG\r\n\x1a\n' else 'DIB'})
        return {'kind': 'ICO', 'images': images}
    if data[:2] == b'BM':
        file_bytes, reserved, pixel_offset, dib = unpack('<IIII', data, 2)
        if pixel_offset > len(data):
            raise ValueError('BMP pixels exceed container')
        width, height, planes, bits = unpack('<iiHH', data, 18) if dib >= 40 else unpack('<HHHH', data, 18)
        return {'kind': 'BMP', 'declared_bytes': file_bytes, 'actual_bytes': len(data), 'dib_bytes': dib,
                'width': width, 'height': height, 'planes': planes, 'bits': bits, 'pixel_offset': pixel_offset}
    if data[:4] == b'DDS ':
        header_size, flags, height, width, pitch, depth, mipmaps = unpack('<IIIIIII', data, 4)
        if header_size != 124 or len(data) < 128:
            raise ValueError('Invalid DDS header')
        pixel_format_bytes, pixel_flags, four_cc = unpack('<II4s', data, 76)
        caps, caps2 = unpack('<II', data, 108)
        return {'kind': 'DDS', 'width': width, 'height': height, 'depth': depth, 'mipmaps': mipmaps,
                'flags': flags, 'four_cc': four_cc.hex(), 'pixel_flags': pixel_flags, 'caps': caps, 'caps2': caps2}
    if data[:2] in (b'II', b'MM'):
        endian = '<' if data[:2] == b'II' else '>'
        version, offset = unpack(endian+'HI', data, 2)
        if version != 42:
            return {'kind': 'TIFF family', 'version': version, 'header_only': True}
        count, = unpack(endian+'H', data, offset)
        if offset+2+count*12+4 > len(data):
            raise ValueError('TIFF directory exceeds container')
        tags = []
        for index in range(count):
            tag, kind, elements, value = unpack(endian+'HHII', data, offset+2+index*12)
            if tag in (256,257,258,259,270,271,272,305,315,33432):
                entry = {'tag': tag, 'type': kind, 'count': elements, 'value_or_offset': value}
                if kind == 2:
                    start = offset+2+index*12+8 if elements <= 4 else value
                    if start+elements > len(data):
                        raise ValueError('TIFF notice exceeds container')
                    entry['text'] = data[start:start+elements].rstrip(b'\0').decode('utf-8', errors='replace')
                tags.append(entry)
        return {'kind': 'TIFF', 'endian': endian, 'first_ifd_tags': tags}
    return {'kind': 'unclassified container', 'signature': data[:16].hex(), 'manual_inspection_required': True}

raw = MANIFEST.read_bytes()
if hashlib.sha256(raw).hexdigest() != PIN:
    raise ValueError('Approved manifest changed')
items = json.loads(raw)['items']
results, errors, missing = [], [], []
for item in items:
    path = SNAPSHOT / item['path']
    if not path.exists():
        missing.append({'path': item['path'], 'sha256': item['payload_sha256'], 'bytes': item['payload_bytes']})
        continue
    data = path.read_bytes()
    actual = hashlib.sha256(data).hexdigest()
    if actual != item['payload_sha256'] or len(data) != item['payload_bytes']:
        raise ValueError('Snapshot differs from approved payload: '+item['path'])
    row = {'path': item['path'], 'bytes': len(data), 'sha256': actual}
    try:
        row['container'] = inspect(data, path.suffix.lower())
    except (ValueError, KeyError, UnicodeError) as error:
        row['inspection_error'] = str(error)
        errors.append({'path': item['path'], 'error': str(error)})
    results.append(row)
report = {'status': 'static container/name-table inspection only', 'manifest_sha256': PIN, 'snapshot': str(SNAPSHOT),
          'paths': len(results), 'bytes': sum(row['bytes'] for row in results),
          'kinds': dict(collections.Counter(row.get('container', {}).get('kind', 'inspection error') for row in results)),
          'errors': errors, 'missing': missing, 'payloads': results,
          'limits': ['No image decode/render, font instantiation, engine import or executable payload was run.',
                     'Headers and embedded notices do not decide legal, visual, product, renderer or AP acceptance.']}
(OUT/'payload-inspection.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
font_notices = [row for row in results if row.get('container', {}).get('kind') == 'sfnt font']
(SNAPSHOT/'notices/payload-font-name-notices.json').write_text(json.dumps(font_notices, indent=2)+'\n', encoding='utf-8')
print(json.dumps({'paths': len(results), 'bytes': report['bytes'], 'kinds': report['kinds'], 'inspection_errors': errors}))

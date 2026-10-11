"""Exact parent-assigned wrapper guard; preserve all original bytes outside the insertion."""
import hashlib
import json
from pathlib import Path
OUT = Path(__file__).resolve().parent
TARGET = Path('F:/JericoEngine/JericoEngine/cmake/3rdParty/FindGoogleBenchmark.cmake')
before = TARGET.read_bytes()
if hashlib.sha256(before).hexdigest()!='332858760a3ebd76652f99f403e8ec232533bb1f2e1a70aabae8310bcce278ab':
    raise ValueError('Reviewed wrapper changed; preserve concurrent edits and stop')
old = b'    FetchContent_MakeAvailable(googlebenchmark)\r\n'
insertion = (b'    # Archive sources must not query an ancestor repository or refresh its Git index.\r\n'
             b'    # Shadow inherited/cache executables: upstream checks GIT_EXECUTABLE directly.\r\n'
             b'    set(CMAKE_DISABLE_FIND_PACKAGE_Git ON)\r\n'
             b'    set(GIT_EXECUTABLE "")\r\n'
             b'\r\n')
if before.count(old)!=1:
    raise ValueError('Reviewed insertion is not unique')
after = before.replace(old,insertion+old)
TARGET.write_bytes(after)
record = {'path':str(TARGET),'before_sha256':hashlib.sha256(before).hexdigest(),'after_sha256':hashlib.sha256(after).hexdigest(),
          'before_bytes':len(before),'after_bytes':len(after),'inserted_text':insertion.decode(),
          'all_other_bytes_unchanged':after.replace(insertion,b'')==before,'original_bom':before.startswith(b'\xef\xbb\xbf'),
          'before_crlf':before.count(b'\r\n'),'after_crlf':after.count(b'\r\n'),'bare_lf_before':before.count(b'\n')-before.count(b'\r\n'),
          'bare_lf_after':after.count(b'\n')-after.count(b'\r\n'),'configure_or_test_executed':False}
(OUT/'wrapper-guard-change.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
print(json.dumps(record))

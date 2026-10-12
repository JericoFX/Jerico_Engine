import json, sqlite3, hashlib
from pathlib import Path
from datetime import datetime, timezone

root = Path(r'F:\JericoEngine\JericoEngine')
cache = root / 'Projects/JEEditorFixture/Cache'
connection = sqlite3.connect('file:' + (cache / 'assetdb.sqlite').as_posix() + '?mode=ro', uri=True)
connection.row_factory = sqlite3.Row
connection.execute('PRAGMA query_only=ON')
condition = "(s.SourceName LIKE '%JEEditorFixture.prefab' OR s.SourceName LIKE '%JENativeIntegration.prefab' OR s.SourceName LIKE 'Shaders/ImGuiAtom/%')"
def rows(query):
    return [{k: (v.hex().upper() if isinstance(v, bytes) else v) for k, v in dict(row).items()} for row in connection.execute(query)]
sources = rows('SELECT s.*, f.ScanFolder, f.PortableKey FROM Sources s JOIN ScanFolders f ON s.ScanFolderPK=f.ScanFolderID WHERE ' + condition)
jobs = rows('SELECT s.SourceName,j.* FROM Sources s JOIN Jobs j ON j.SourcePK=s.SourceID WHERE ' + condition)
products = rows('SELECT s.SourceName,p.* FROM Sources s JOIN Jobs j ON j.SourcePK=s.SourceID JOIN Products p ON p.JobPK=j.JobID WHERE ' + condition)
folders = rows("SELECT * FROM ScanFolders WHERE PortableKey LIKE '%5F77588E%' OR ScanFolder LIKE '%Render/Debug%'")
connection.close()
for product in products:
    path = cache / product['ProductName']
    if path.is_file():
        product['bytes'] = path.stat().st_size
        product['sha256'] = hashlib.sha256(path.read_bytes()).hexdigest().upper()
        product['lastWriteUtc'] = datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).isoformat()
print(json.dumps({'capturedUtc': datetime.now(timezone.utc).isoformat(), 'mode': 'read-only SQLite, query_only', 'scanFolders': folders, 'sources': sources, 'jobs': jobs, 'products': products}, indent=2))

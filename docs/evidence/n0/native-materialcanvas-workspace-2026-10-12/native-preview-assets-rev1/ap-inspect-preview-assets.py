import json, sqlite3, hashlib
from pathlib import Path
from datetime import datetime, timezone
root=Path(r'F:\JericoEngine\JericoEngine')
db=root/'Projects/JEEditorFixture/Cache/assetdb.sqlite'
c=sqlite3.connect('file:'+db.as_posix()+'?mode=ro',uri=True)
c.row_factory=sqlite3.Row
c.execute('PRAGMA query_only=ON')
def rows(q,args=()):
    return [{k:(v.hex().upper() if isinstance(v,bytes) else v) for k,v in dict(r).items()} for r in c.execute(q,args)]
scan_cols=rows('PRAGMA table_info(ScanFolders)')
source_cols=rows('PRAGMA table_info(Sources)')
product_cols=rows('PRAGMA table_info(Products)')
job_cols=rows('PRAGMA table_info(Jobs)')
folders=rows("SELECT ScanFolderID,ScanFolder,DisplayName,PortableKey,IsRoot FROM ScanFolders WHERE ScanFolder LIKE '%MaterialEditor%' OR ScanFolder LIKE '%MaterialCanvas%' OR PortableKey LIKE '%BA3076F4-E9F4-5243-9754-E19A40F6B90C%' OR PortableKey LIKE '%0CF3D5DF-4FD1-5108-B937-DC20B6A53B48%' ORDER BY ScanFolderID")
# Scope to the two relevant scan folders and preview-related source names; preserve exact source and product identities.
ids=[r['ScanFolderID'] for r in folders]
if ids:
    marks=','.join('?' for _ in ids)
    sources=rows(f"SELECT s.SourceID,s.ScanFolderPK,f.ScanFolder,f.DisplayName,f.PortableKey,s.SourceName,hex(s.SourceGuid) AS SourceGuid FROM Sources s JOIN ScanFolders f ON f.ScanFolderID=s.ScanFolderPK WHERE s.ScanFolderPK IN ({marks}) AND (s.SourceName LIKE '%MaterialEditor%' OR s.SourceName LIKE '%MaterialCanvas%' OR s.SourceName LIKE '%ViewportModels%' OR s.SourceName LIKE '%LightingPresets%' OR s.SourceName LIKE '%shaderball%' OR s.SourceName LIKE '%konzerthaus%') ORDER BY f.ScanFolder,s.SourceName",ids)
else: sources=[]
sourceids=[r['SourceID'] for r in sources]
if sourceids:
    marks=','.join('?' for _ in sourceids)
    jobs=rows(f"SELECT s.SourceID,s.SourceName,hex(s.SourceGuid) AS SourceGuid,j.JobID,j.JobKey,j.Platform,j.Status,j.JobRunKey,j.ErrorCount,j.WarningCount,j.FirstFailLogTime,j.FirstFailLogFile,j.LastFailLogTime,j.LastFailLogFile,j.LastLogTime,j.LastLogFile FROM Jobs j JOIN Sources s ON s.SourceID=j.SourcePK WHERE s.SourceID IN ({marks}) ORDER BY s.SourceName,j.Platform,j.JobKey",sourceids)
    products=rows(f"SELECT s.SourceID,s.SourceName,hex(s.SourceGuid) AS SourceGuid,p.ProductID,p.SubID,p.ProductName,p.AssetType,p.JobPK FROM Products p JOIN Jobs j ON j.JobID=p.JobPK JOIN Sources s ON s.SourceID=j.SourcePK WHERE s.SourceID IN ({marks}) ORDER BY s.SourceName,p.ProductName",sourceids)
else: jobs=[]; products=[]
files=[]
for r in sources:
    p=Path(r['ScanFolder'])/r['SourceName']
    item={'sourceId':r['SourceID'],'sourceName':r['SourceName'],'path':str(p),'exists':p.is_file()}
    if p.is_file():
        h=hashlib.sha256(); n=0
        with p.open('rb') as f:
            for chunk in iter(lambda:f.read(1024*1024),b''):
                h.update(chunk); n+=len(chunk)
        item.update(bytes=n,sha256=h.hexdigest().upper())
    files.append(item)
dbfiles=[]
for p in (db,Path(str(db)+'-wal'),Path(str(db)+'-shm')):
    if p.exists():
        h=hashlib.sha256(); n=0
        with p.open('rb') as f:
            for chunk in iter(lambda:f.read(1024*1024),b''):
                h.update(chunk); n+=len(chunk)
        dbfiles.append({'path':str(p),'bytes':n,'sha256':h.hexdigest().upper(),'lastWriteUtc':datetime.fromtimestamp(p.stat().st_mtime,timezone.utc).isoformat()})
c.close()
report={'capturedUtc':datetime.now(timezone.utc).isoformat(),'mode':'SQLite URI mode=ro; PRAGMA query_only=ON','dbFiles':dbfiles,'scanFolderColumns':scan_cols,'sourceColumns':source_cols,'productColumns':product_cols,'jobColumns':job_cols,'scanFolders':folders,'sourceIdentities':sources,'sourceFiles':files,'products':products,'jobs':jobs}
print(json.dumps(report,indent=2))

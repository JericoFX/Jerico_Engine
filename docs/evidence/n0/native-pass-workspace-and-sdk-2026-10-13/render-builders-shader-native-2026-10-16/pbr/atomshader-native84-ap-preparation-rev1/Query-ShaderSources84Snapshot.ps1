param([switch]$Execute,[string]$DatabaseCopyPath,[string]$ResultPath)
$ErrorActionPreference='Stop'
if(!$Execute){Write-Output 'PREPARED_NOT_RUN: private snapshot query only, assigned validation owner required.';return}
if(!$DatabaseCopyPath -or !$ResultPath){throw 'Explicit private database copy and new result required.'}
if(Test-Path -LiteralPath $resultPath){throw 'Preserve prior result; no overwrite.'}
$dll='C:/Windows/System32/winsqlite3.dll'
if((Get-FileHash -LiteralPath $dll).Hash -cne 'A9148FBBBB59E40E0BE60B6F0F85D286AC55A4558A8A3B1E4CCDCAC41BFACFAB'){throw 'Builtin SQLite identity drift.'}
$dbPath=[IO.Path]::GetFullPath($DatabaseCopyPath)
$allowed=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent)).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
if(!$dbPath.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFullPath($ResultPath) -ine (Join-Path ([IO.Path]::GetDirectoryName($dbPath)) 'readonly-query.json')){throw 'Query only the private snapshot, never live database.'}
$os=Get-CimInstance Win32_OperatingSystem
if(100*(1-$os.FreePhysicalMemory/$os.TotalVisibleMemorySize) -ge 80){throw 'Global RAM guard.'}
if('Jerico.PrivateDiagnostics.SqliteReadonly44' -as [type]){throw 'Use fresh host, not an unknown existing probe type.'}
$nativeCode=@'
using System;
using System.Runtime.InteropServices;
using System.Text;
namespace Jerico.PrivateDiagnostics {
 public static class SqliteReadonly44 {
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_open_v2([MarshalAs(UnmanagedType.LPUTF8Str)] string path,out IntPtr db,int flags,IntPtr vfs);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_close(IntPtr db);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern IntPtr sqlite3_errmsg(IntPtr db);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_prepare_v2(IntPtr db,[MarshalAs(UnmanagedType.LPUTF8Str)] string sql,int count,out IntPtr statement,IntPtr tail);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_stmt_readonly(IntPtr statement);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_step(IntPtr statement);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_finalize(IntPtr statement);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_column_count(IntPtr statement);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern IntPtr sqlite3_column_name(IntPtr statement,int column);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_column_type(IntPtr statement,int column);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern long sqlite3_column_int64(IntPtr statement,int column);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern double sqlite3_column_double(IntPtr statement,int column);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern IntPtr sqlite3_column_text(IntPtr statement,int column);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern IntPtr sqlite3_column_blob(IntPtr statement,int column);
  [DllImport("winsqlite3.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
  [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
  public static extern int sqlite3_column_bytes(IntPtr statement,int column);
  public static byte[] Copy(IntPtr pointer,int count) {
   if(count<0 || count>1048576) throw new InvalidOperationException("Column exceeds bounded diagnostic size.");
   byte[] bytes=new byte[count]; if(count>0) Marshal.Copy(pointer,bytes,0,count); return bytes;
  }
  public static string Z(IntPtr pointer) {
   if(pointer==IntPtr.Zero) return null;
   int count=0; while(count<4096 && Marshal.ReadByte(pointer,count)!=0) count++;
   if(count==4096) throw new InvalidOperationException("Unbounded SQLite identifier/error.");
   return Encoding.UTF8.GetString(Copy(pointer,count));
  }
 }
}
'@
$before=@(foreach($p in @($dbPath,($dbPath+'-wal'),($dbPath+'-shm'))){if(!(Test-Path -LiteralPath $p -PathType Leaf)){continue};$f=Get-Item -LiteralPath $p;[ordered]@{path=$p;bytes=$f.Length;lastWriteUtc=$f.LastWriteTimeUtc.ToString('o')}})
$results=[Collections.Generic.List[object]]::new()
$db=[IntPtr]::Zero; $openResult=$null; $closeResult=$null; $failure=$null
$started=[DateTime]::UtcNow
try{
 Add-Type -TypeDefinition $nativeCode -Language CSharp
 # SQLITE_OPEN_READONLY=1 | SQLITE_OPEN_FULLMUTEX=0x10000. No CREATE/READWRITE/URI.
 $openResult=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_open_v2($dbPath,[ref]$db,65537,[IntPtr]::Zero)
 if($openResult -ne 0){throw ([Jerico.PrivateDiagnostics.SqliteReadonly44]::Z([Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_errmsg($db)))}
 $workloadPath=Join-Path $PSScriptRoot 'workload.json'
 $sourcePaths=@((Get-Content -LiteralPath $workloadPath -Raw|ConvertFrom-Json).sources.path)
 if($sourcePaths.Count -ne 7){throw 'Exact seven source/watchfolder paths required.'}
 $names=@($sourcePaths|ForEach-Object { "'"+[IO.Path]::GetFullPath($_).Replace('\','/').ToLowerInvariant().Replace("'","''")+"'" })
 $ids="SELECT s.SourceID FROM Sources s JOIN ScanFolders f ON s.ScanFolderPK=f.ScanFolderID WHERE lower(replace(f.ScanFolder,'\','/') || '/' || replace(s.SourceName,'\','/')) IN ("+($names -join ',')+")"
 $filter="s.SourceID IN ($ids)"
 $queries=@(
  @{name='schema';maxRows=16;sql="SELECT name,sql FROM sqlite_master WHERE type='table' AND name IN ('Sources','Products','Jobs','ScanFolders','ProductDependencies','SourceDependency') ORDER BY name LIMIT 17;"},
  @{name='sources';maxRows=7;sql="SELECT s.*,f.ScanFolder,f.PortableKey FROM Sources s JOIN ScanFolders f ON s.ScanFolderPK=f.ScanFolderID WHERE $filter ORDER BY s.SourceID LIMIT 8;"},
  @{name='jobs';maxRows=128;sql="SELECT s.SourceName,hex(s.SourceGuid) AS SourceGuidHex,j.* FROM Jobs j JOIN Sources s ON j.SourcePK=s.SourceID WHERE $filter ORDER BY j.JobID LIMIT 129;"},
  @{name='products';maxRows=512;sql="SELECT s.SourceName,hex(s.SourceGuid) AS SourceGuidHex,j.Platform,j.JobKey,j.JobRunKey,p.* FROM Products p JOIN Jobs j ON p.JobPK=j.JobID JOIN Sources s ON j.SourcePK=s.SourceID WHERE $filter ORDER BY p.ProductID LIMIT 513;"},
  @{name='scanFolders';maxRows=7;sql="SELECT f.* FROM ScanFolders f WHERE EXISTS(SELECT 1 FROM Sources s WHERE $filter AND s.ScanFolderPK=f.ScanFolderID) ORDER BY f.ScanFolderID LIMIT 8;"},
  @{name='productDependencies';maxRows=2048;sql="SELECT s.SourceName,d.* FROM ProductDependencies d JOIN Products p ON d.ProductPK=p.ProductID JOIN Jobs j ON p.JobPK=j.JobID JOIN Sources s ON j.SourcePK=s.SourceID WHERE $filter ORDER BY d.ProductDependencyID LIMIT 2049;"},
  @{name='sourceDependencies';maxRows=2048;sql="SELECT s.SourceName,d.* FROM SourceDependency d JOIN Sources s ON d.SourceGuid=s.SourceGuid WHERE $filter ORDER BY d.SourceDependencyID LIMIT 2049;"},
  @{name='globalNonCompleted';maxRows=4096;sql="SELECT s.SourceName,hex(s.SourceGuid) AS SourceGuidHex,j.* FROM Jobs j JOIN Sources s ON j.SourcePK=s.SourceID WHERE j.Status != 4 ORDER BY s.SourceName,j.Platform,j.JobKey,j.BuilderGuid LIMIT 4097;"}
 )
 foreach($q in $queries){
  $os=Get-CimInstance Win32_OperatingSystem
  if(100*(1-$os.FreePhysicalMemory/$os.TotalVisibleMemorySize) -ge 80){throw 'Global RAM guard before next SELECT.'}
  $statement=[IntPtr]::Zero
  $rows=[Collections.Generic.List[object]]::new()
  $item=[ordered]@{name=$q.name;sql=$q.sql;startedUtc=[DateTime]::UtcNow.ToString('o');statementReadonly=$false;prepareResult=$null;stepResult=$null;finalizeResult=$null;error=$null;rows=@()}
  try{
   if($q.sql -notmatch '^SELECT '){throw 'Only fixed SELECT statements allowed.'}
   $item.prepareResult=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_prepare_v2($db,$q.sql,-1,[ref]$statement,[IntPtr]::Zero)
   if($item.prepareResult -ne 0){throw ([Jerico.PrivateDiagnostics.SqliteReadonly44]::Z([Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_errmsg($db)))}
   $item.statementReadonly=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_stmt_readonly($statement) -eq 1
   if(!$item.statementReadonly){throw 'Statement is not readonly; not stepped.'}
   $columns=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_count($statement)
   while(($item.stepResult=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_step($statement)) -eq 100){
    if($rows.Count -ge $q.maxRows){throw 'Unexpected query row budget.'}
    $row=[ordered]@{}
    for($c=0;$c -lt $columns;$c++){
     $name=[Jerico.PrivateDiagnostics.SqliteReadonly44]::Z([Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_name($statement,$c))
     $kind=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_type($statement,$c)
     $value=$null
     switch($kind){
      1 {$value=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_int64($statement,$c)}
      2 {$value=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_double($statement,$c)}
      3 {$length=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_bytes($statement,$c);$pointer=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_text($statement,$c);$bytes=[Jerico.PrivateDiagnostics.SqliteReadonly44]::Copy($pointer,$length);$value=[Text.Encoding]::UTF8.GetString($bytes)}
      4 {$length=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_bytes($statement,$c);$pointer=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_column_blob($statement,$c);$bytes=[Jerico.PrivateDiagnostics.SqliteReadonly44]::Copy($pointer,$length);$value=[ordered]@{kind='blob';bytes=$length;hex=[BitConverter]::ToString($bytes).Replace('-','')}}
      5 {$value=$null}
      default {throw 'Unknown SQLite column type.'}
     }
     $row[$name]=$value
    }
    $rows.Add($row)
   }
   if($item.stepResult -ne 101){throw ([Jerico.PrivateDiagnostics.SqliteReadonly44]::Z([Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_errmsg($db)))}
  }catch{$item.error=$_.ToString()}
  finally{
   if($statement -ne [IntPtr]::Zero){$item.finalizeResult=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_finalize($statement);$statement=[IntPtr]::Zero}
   $item.rows=$rows.ToArray();$results.Add($item)
  }
 }
}catch{$failure=$_.ToString()}
finally{
 if($db -ne [IntPtr]::Zero){$closeResult=[Jerico.PrivateDiagnostics.SqliteReadonly44]::sqlite3_close($db);$db=[IntPtr]::Zero}
 $report=[ordered]@{state='READONLY_QUERY_OBSERVATION_NOT_ACCEPTANCE';startedUtc=$started.ToString('o');endedUtc=[DateTime]::UtcNow.ToString('o');failure=$failure;openFlags=65537;openResult=$openResult;closeResult=$closeResult;dbPath=$dbPath;filesBefore=$before;queries=$results.ToArray();nativeDll=$dll;nativeDllSHA256=(Get-FileHash -LiteralPath $dll).Hash;scriptSHA256=(Get-FileHash -LiteralPath $PSCommandPath).Hash;scope='Seven exact source/watchfolder SELECTs on a stable copied main/WAL/SHM family. No immutable flag, no PRAGMA/checkpoint or live DB writes. Exact ProductPK and SourceGuid joins from the six-table parent schema are captured; no inferred dependency resolution. GlobalNonCompleted is raw status data, not automatically classified as347LFS.'}
 $stream=[IO.File]::Open($resultPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
 try{$bytes=[Text.UTF8Encoding]::new($false).GetBytes(($report|ConvertTo-Json -Depth 14));$stream.Write($bytes,0,$bytes.Length)}
 finally{$stream.Dispose()}
}
if($failure -or $closeResult -ne 0 -or @($results|Where-Object {$_.error -or !$_.statementReadonly}).Count){Write-Error 'Readonly query incomplete; inspect result.';exit 1}
Write-Output ('Read-only bounded queries complete; result='+$resultPath)
exit 0

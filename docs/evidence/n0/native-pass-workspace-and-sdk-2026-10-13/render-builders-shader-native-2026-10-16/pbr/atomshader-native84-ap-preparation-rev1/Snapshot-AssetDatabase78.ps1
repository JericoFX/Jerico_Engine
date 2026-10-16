function Save-AssetDatabase78Snapshot([string]$label) {
    $active = @(Get-CimInstance Win32_Process | Where-Object { $_.Name -in @('AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','Editor.exe') })
    if ($active.Count) { throw 'Database snapshot requires no AP/Builder/Editor process.' }
    $database = Join-Path $repo 'Projects/JEEditorFixture/Cache/assetdb.sqlite'
    $directory = Join-Path $evidence ($label + '-database')
    if (Test-Path -LiteralPath $directory) { throw 'Preserve existing database snapshot.' }
    $family = @($database, ($database + '-wal'), ($database + '-shm'))
    $before = @(foreach ($path in $family) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $item = Get-Item -LiteralPath $path
            [pscustomobject]@{path=$path;bytes=$item.Length;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
        }
    })
    if (@($before | Where-Object path -ceq $database).Count -ne 1) { throw 'Asset database missing.' }
    [void][IO.Directory]::CreateDirectory($directory)
    foreach ($pin in $before) {
        [IO.File]::Copy($pin.path, (Join-Path $directory ([IO.Path]::GetFileName($pin.path))), $false)
    }
    $after = @(foreach ($path in $family) { if (Test-Path -LiteralPath $path -PathType Leaf) { Get-Item -LiteralPath $path } })
    if ($after.Count -ne $before.Count) { throw 'Database family changed while copying.' }
    foreach ($pin in $before) {
        $copy = Join-Path $directory ([IO.Path]::GetFileName($pin.path))
        if ((Get-Item -LiteralPath $pin.path).Length -ne $pin.bytes -or (Get-FileHash -LiteralPath $pin.path -Algorithm SHA256).Hash -cne $pin.sha256 -or (Get-Item -LiteralPath $copy).Length -ne $pin.bytes -or (Get-FileHash -LiteralPath $copy -Algorithm SHA256).Hash -cne $pin.sha256) { throw 'Database snapshot is not stable and byte exact.' }
    }
    Save-Json (Join-Path $directory 'family-snapshot.json') @{sourcePins=$before;walPreserved=$true;immutable=$false;scope='Stable closed main/WAL/SHM family, no checkpoint or live database write. Query only this private copy using the existing readonly SQLite provider.'}
    return (Join-Path $directory 'assetdb.sqlite')
}

function Observe-AssetModules78($snapshot, [string]$base) {
    foreach ($row in @($snapshot.processes | Where-Object { $_.observedPhaseDescendant -and $_.name -in @('AssetProcessorBatch.exe','AssetBuilder.exe') })) {
        $key = "$($row.pid)|$($row.creationUtc)"
        if (!$script:moduleCaptureKeys.Add($key)) { continue }
        $observation = [ordered]@{pid=$row.pid;creationUtc=$row.creationUtc;executable=$row.executable;state='NOT_OBSERVED';modules=@();verifiedNativePins=@();legacyBuilderObserved=$false;backendLibrariesObserved=@();pythonLibrariesObserved=@();nativeActivation='NOT_OBSERVED';error=$null;scope='One point-in-time capture per owned PID+creation, not full-lifetime absence or active graphics-device proof.'}
        try {
            $current = Get-CimInstance Win32_Process -Filter "ProcessId=$($row.pid)"
            if (!$current -or $current.ExecutablePath -ine $row.executable -or [Math]::Abs(($current.CreationDate.ToUniversalTime() - ([DateTimeOffset]$row.creationUtc).UtcDateTime).TotalMilliseconds) -ge 1) { throw 'Process ended or identity changed before module capture.' }
            $process = Get-Process -Id $row.pid -ErrorAction Stop
            try { if([Math]::Abs(($process.StartTime.ToUniversalTime()-([DateTimeOffset]$row.creationUtc).UtcDateTime).TotalMilliseconds) -ge 1){throw 'Process handle creation differs.'}; $modules = @($process.Modules | ForEach-Object { [pscustomobject]@{name=$_.ModuleName;path=$_.FileName} }) } finally { $process.Dispose() }
            $observation.modules = $modules
            $observation.state = 'CAPTURED'
            $observation.legacyBuilderObserved = @($modules | Where-Object { $_.name -in @('Atom_RPI.Builders.dll','AtomShader.Builders.dll') }).Count -gt 0
            $observation.backendLibrariesObserved = @($modules | Where-Object name -match '^Atom_RHI_(?:Vulkan|DX12|Metal|Null)')
            $observation.pythonLibrariesObserved = @($modules | Where-Object name -match '(?i)python')
            foreach ($name in @('RenderAssetBuilders.dll','Atom_RPI.Public.dll')) {
                $loaded = @($modules | Where-Object name -ieq $name)
                if ($loaded.Count -ne 1) { continue }
                $expected = @($hostReview.binaryPins | Where-Object name -ceq $name)
                $item = Get-Item -LiteralPath $loaded[0].path
                $hash = (Get-FileHash -LiteralPath $loaded[0].path -Algorithm SHA256).Hash
                if ($expected.Count -ne 1 -or [IO.Path]::GetFullPath($item.FullName) -ine [IO.Path]::GetFullPath($expected[0].path) -or $item.Length -ne $expected[0].bytes -or $hash -cne $expected[0].sha256) { throw "Loaded native module differs from source82 pin: $name" }
                $observation.verifiedNativePins += @{name=$name;path=$item.FullName;bytes=$item.Length;sha256=$hash}
            }
            $observation.nativeActivation = if ($observation.verifiedNativePins.Count -eq 2) { 'BOTH_NATIVE_DLLS_OBSERVED_AND_PINNED' } else { 'NOT_OBSERVED' }
        } catch { $observation.error=$_.Exception.Message; $observation.state=if($observation.error.StartsWith('Loaded native module differs')){'MISMATCH'}else{'NOT_OBSERVED'} }
        [IO.File]::AppendAllText(($base+'.modules.jsonl'),($observation|ConvertTo-Json -Depth 7 -Compress)+[Environment]::NewLine,[Text.UTF8Encoding]::new($false))
    }
}

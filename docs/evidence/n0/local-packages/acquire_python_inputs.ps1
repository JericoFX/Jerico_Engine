$ErrorActionPreference = 'Stop'
$pythonInputPins = @(
    @{name='cpython'; repository='python/cpython'; commit='49965601d6afedafe47cc85556d99b7a24981051'},
    @{name='expat'; repository='libexpat/libexpat'; commit='65a21f2b2a306d29b44e70264aca948aa0454219'},
    @{name='bzip2-1.0.8'; repository='python/cpython-source-deps'; commit='05301997b2f9590f49c672cf3dfd3d3dfa7ad521'},
    @{name='sqlite-3.40.1.0'; repository='python/cpython-source-deps'; commit='db4ca556b25787c263ee72a4b764c4be474c322f'},
    @{name='xz-5.2.5'; repository='python/cpython-source-deps'; commit='c6bc0c612605622aaef101a33a751f9de2ecc193'},
    @{name='zlib-1.2.13'; repository='python/cpython-source-deps'; commit='9071e5ae1bd90c30518404b95ec0be0d4cf6cf84'},
    @{name='libffi-3.3.0'; repository='python/cpython-bin-deps'; commit='1cf06233e3ceb49dc0a73c55e04b1174b436b632'},
    @{name='openssl-bin-1.1.1u'; repository='python/cpython-bin-deps'; commit='bbd07240c6414fa314ca606b73247054eb81b59c'},
    @{name='tcltk-8.6.12.0'; repository='python/cpython-bin-deps'; commit='e3c3e9a2856124aa32b608632a52742d479eb7a9'}
)
$pythonAcquisition = [System.Collections.Generic.List[object]]::new()
$pythonHandler = [System.Net.Http.HttpClientHandler]::new()
$pythonHandler.AllowAutoRedirect = $false
$pythonClient = [System.Net.Http.HttpClient]::new($pythonHandler)
$pythonClient.Timeout = [TimeSpan]::FromMinutes(5)
try {
    foreach ($pythonPin in $pythonInputPins) {
        $pythonUri = [Uri]("https://codeload.github.com/{0}/tar.gz/{1}" -f $pythonPin.repository, $pythonPin.commit)
        if ($pythonUri.Scheme -ne 'https' -or $pythonUri.Host -ne 'codeload.github.com' -or $pythonUri.Query -or $pythonUri.Fragment) { throw 'Unaudited acquisition destination' }
        $pythonArchive = Join-Path 'F:\JericoEngine\Dependencies\archives' ("{0}-{1}.tar.gz" -f $pythonPin.name, $pythonPin.commit)
        if (Test-Path -LiteralPath $pythonArchive) { throw "Archive already exists: $pythonArchive" }
        $pythonStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $pythonResponse = $pythonClient.GetAsync($pythonUri, [System.Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
        try {
            if ([int]$pythonResponse.StatusCode -ne 200 -or $pythonResponse.Headers.Location) { throw "Acquisition did not return direct 200: $($pythonResponse.StatusCode)" }
            $pythonStream = [System.IO.File]::Open($pythonArchive, [System.IO.FileMode]::CreateNew)
            try { $pythonResponse.Content.CopyToAsync($pythonStream).GetAwaiter().GetResult() } finally { $pythonStream.Dispose() }
        } finally { $pythonResponse.Dispose() }
        $pythonStopwatch.Stop()
        $pythonReceipt = [ordered]@{
            name=$pythonPin.name; repository=$pythonPin.repository; commit=$pythonPin.commit; url=$pythonUri.AbsoluteUri;
            archive=$pythonArchive; sha256=(Get-FileHash -LiteralPath $pythonArchive -Algorithm SHA256).Hash.ToLowerInvariant();
            bytes=(Get-Item -LiteralPath $pythonArchive).Length; elapsed_seconds=$pythonStopwatch.Elapsed.TotalSeconds;
            redirect_followed=$false; code_executed=$false; archive_hash_is_acquisition_receipt_not_independent_signature=$true
        }
        $pythonAcquisition.Add($pythonReceipt)
        $pythonAcquisition.ToArray() | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'python-input-acquisition.json') -Encoding utf8
        Write-Output ("{0}: {1} bytes SHA256 {2}" -f $pythonReceipt.name, $pythonReceipt.bytes, $pythonReceipt.sha256)
    }
} finally { $pythonClient.Dispose(); $pythonHandler.Dispose() }

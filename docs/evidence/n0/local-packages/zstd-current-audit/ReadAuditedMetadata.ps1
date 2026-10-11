param([Parameter(Mandatory=$true)][string]$SourceUrl,[Parameter(Mandatory=$true)][string]$OutputStem)
foreach ($method in @('HEAD','GET')) {
    $request=[System.Net.HttpWebRequest]::Create($SourceUrl)
    $request.Method=$method
    $request.AllowAutoRedirect=$false
    $request.Timeout=30000
    $request.Credentials=$null
    $request.UseDefaultCredentials=$false
    $request.UserAgent='JE-local-dependency-audit'
    $response=$request.GetResponse()
    try {
        $headers=[ordered]@{}
        foreach ($name in $response.Headers.AllKeys) { $headers[$name]=$response.Headers[$name] }
        $receipt=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');method=$method;url=$SourceUrl;status=[int]$response.StatusCode;response_uri=$response.ResponseUri.AbsoluteUri;auto_redirect=$false;credentials='none';headers=$headers}
        if ([int]$response.StatusCode -ne 200 -or $response.ResponseUri.AbsoluteUri -ne $SourceUrl) { $receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($OutputStem+'-'+$method.ToLowerInvariant()+'.json') -Encoding utf8NoBOM; throw 'Redirect/status requires manual destination audit before GET.' }
        if ($method -eq 'GET') {
            $reader=[System.IO.StreamReader]::new($response.GetResponseStream())
            try {$body=$reader.ReadToEnd()} finally {$reader.Dispose()}
            if (Test-Path -LiteralPath ($OutputStem+'.txt')) {throw 'Metadata output exists; refusing overwrite'}
            [System.IO.File]::WriteAllText(($OutputStem+'.txt'),$body,[System.Text.UTF8Encoding]::new($false))
            $receipt.body_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath ($OutputStem+'.txt')).Hash
        }
        $receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($OutputStem+'-'+$method.ToLowerInvariant()+'.json') -Encoding utf8NoBOM
    } finally {$response.Dispose()}
}

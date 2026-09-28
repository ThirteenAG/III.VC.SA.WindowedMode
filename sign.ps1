param(
    [string]$Path = "$PSScriptRoot\data\III.VC.SA.WindowedMode.asi",
    [switch]$VerifyTrust
)

$ErrorActionPreference = 'Stop'
if (-not $env:CODE_SIGNING_PFX -and -not $env:CODE_SIGNING_PASSWORD) {
    Write-Host 'Signing skipped: no signing credentials configured.'
    exit 0
}
if (-not $env:CODE_SIGNING_PFX -or -not $env:CODE_SIGNING_PASSWORD) {
    throw 'Both CODE_SIGNING_PFX and CODE_SIGNING_PASSWORD must be configured.'
}
$binary = (Get-Item -LiteralPath $Path).FullName
$signtool = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe" |
    Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (-not $signtool) { throw 'signtool.exe was not found.' }
$tempPfx = Join-Path ([IO.Path]::GetTempPath()) ("signing_{0}.pfx" -f [guid]::NewGuid())
try {
    [IO.File]::WriteAllBytes($tempPfx, [Convert]::FromBase64String($env:CODE_SIGNING_PFX))
    & $signtool sign /fd SHA256 /f $tempPfx /p $env:CODE_SIGNING_PASSWORD /tr http://timestamp.digicert.com /td SHA256 $binary
    if ($LASTEXITCODE -ne 0) { throw "Signing failed with exit code $LASTEXITCODE." }
    # Authenticode trust depends on the machine's root certificate store.
    # Like FusionFix, require signing to succeed without requiring public trust.
    if ($VerifyTrust) {
        & $signtool verify /pa $binary
        if ($LASTEXITCODE -ne 0) { throw 'Signature trust verification failed.' }
    }
}
finally {
    if (Test-Path -LiteralPath $tempPfx) { Remove-Item -LiteralPath $tempPfx -Force }
}

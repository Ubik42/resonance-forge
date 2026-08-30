param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

$Destination = Join-Path $RepositoryRoot 'Demo\Plugins\ResonanceForge'
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'ResonanceForge.uplugin') -Destination $Destination -Force
Copy-Item -LiteralPath (Join-Path $RepositoryRoot 'Source') -Destination $Destination -Recurse -Force

Write-Output "已同步插件到 $Destination"

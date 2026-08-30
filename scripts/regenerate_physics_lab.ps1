param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$project = Join-Path $RepositoryRoot 'Demo\ResonanceForgeDemo.uproject'
$generator = Join-Path $RepositoryRoot 'Demo\Scripts\generate_showcase.py'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

$openEditors = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object {
    $_.MainWindowTitle -like 'ResonanceForgeDemo*'
})
if ($openEditors.Count -gt 0) {
    $ids = ($openEditors.Id -join ', ')
    throw "ResonanceForgeDemo 当前仍在编辑器中打开（PID: $ids）。请先正常关闭编辑器，避免覆盖正在编辑的地图。"
}

foreach ($requiredPath in @($project, $generator, $editorCmd)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "缺少必需文件：$requiredPath"
    }
}

& $editorCmd $project "-ExecutePythonScript=$generator" -unattended -nop4 -nosplash -NullRHI
if ($LASTEXITCODE -ne 0) {
    throw "演示地图生成失败，UnrealEditor-Cmd 退出码：$LASTEXITCODE"
}

$report = Join-Path $RepositoryRoot 'Demo\Saved\ResonanceForge\physics_lab_generation.json'
if (-not (Test-Path -LiteralPath $report)) {
    throw "地图生成进程已结束，但没有找到复检报告：$report"
}

$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
$invalidBalls = @($result.physics_balls | Where-Object {
    -not $_.simulate_physics -or -not $_.gravity_enabled -or $_.mobility -notmatch 'MOVABLE'
})
if ($result.status -ne 'success' -or $invalidBalls.Count -gt 0 -or $result.visual_story -notmatch 'Wwise') {
    throw "地图生成后的物理状态复检未通过。请查看：$report"
}

Write-Output "演示材质与地图已重建，3 个落球均已启用 Movable、重力和物理模拟。"
Write-Output "复检报告：$report"

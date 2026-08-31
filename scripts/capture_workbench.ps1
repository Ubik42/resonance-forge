param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [int]$TimeoutSeconds = 55
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repositoryRoot 'Demo\ResonanceForgeDemo.uproject'
$editorPath = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'

if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) {
    throw "未找到 Unreal Editor：$editorPath"
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) {
    throw "未找到演示工程：$projectPath"
}

$existingEditors = @(Get-Process UnrealEditor, UnrealEditor-Cmd -ErrorAction SilentlyContinue)
if ($existingEditors.Count -gt 0) {
    $processSummary = ($existingEditors | ForEach-Object { "$($_.ProcessName) PID $($_.Id)" }) -join '、'
    throw "检测到正在运行的 UE（$processSummary）。为避免同工程会话冲突，本次未启动截图；关闭对应编辑器后重试。"
}

$arguments = @(
    $projectPath,
    '-ExecCmds=ResonanceForge.CaptureWorkbench',
    '-ResonanceForgeCaptureAndExit',
    '-nosplash',
    '-windowed',
    '-ResX=1600',
    '-ResY=1200'
)

$captureProcess = Start-Process -FilePath $editorPath -ArgumentList $arguments -WindowStyle Hidden -PassThru
$deadline = (Get-Date).AddSeconds([Math]::Max(10, $TimeoutSeconds))
while (-not $captureProcess.HasExited -and (Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 500
    $captureProcess.Refresh()
}

if (-not $captureProcess.HasExited) {
    Stop-Process -Id $captureProcess.Id -Force
    throw "工作台截图在 $TimeoutSeconds 秒内未完成；仅终止了本次创建的 PID $($captureProcess.Id)。"
}
if ($captureProcess.ExitCode -ne 0) {
    throw "工作台截图进程异常退出：PID $($captureProcess.Id)，ExitCode $($captureProcess.ExitCode)。"
}

$imageDirectory = Join-Path $repositoryRoot 'docs\images'
$expectedImages = @(
    'resonance-forge-workbench.png',
    'resonance-forge-keybed.png',
    'resonance-forge-workbench-details.png',
    'resonance-forge-mode-rack.png'
)
foreach ($imageName in $expectedImages) {
    $imagePath = Join-Path $imageDirectory $imageName
    if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) {
        throw "截图进程已退出，但缺少预期图片：$imagePath"
    }
}

Write-Output "工作台截图完成：PID $($captureProcess.Id)，4 张图片已更新。"

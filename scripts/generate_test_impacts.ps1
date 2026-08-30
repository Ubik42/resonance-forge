param(
    [string]$OutputDirectory = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Demo\TestAudio\Generated')
)

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function Write-ImpactWave {
    param(
        [string]$Path,
        [double]$Energy,
        [double]$Brightness,
        [int]$Seed
    )

    $sampleRate = 48000
    $durationSeconds = 1.25
    $sampleCount = [int]($sampleRate * $durationSeconds)
    $samples = New-Object 'System.Int16[]' $sampleCount
    $random = [System.Random]::new($Seed)
    $frequencies = @(224.0, 361.0, 587.0, 927.0, 1438.0, 2215.0, 3390.0)

    for ($index = 0; $index -lt $sampleCount; $index++) {
        $time = $index / [double]$sampleRate
        $value = 0.0
        for ($mode = 0; $mode -lt $frequencies.Count; $mode++) {
            $tilt = (1.0 - $Brightness) * [Math]::Pow(0.62, $mode) + $Brightness * [Math]::Pow(0.83, $mode)
            $decay = 3.2 + $mode * 0.72
            $phase = $mode * 0.37
            $value += [Math]::Sin(2.0 * [Math]::PI * $frequencies[$mode] * $time + $phase) * [Math]::Exp(-$decay * $time) * $tilt
        }
        $noise = ($random.NextDouble() * 2.0 - 1.0) * [Math]::Exp(-95.0 * $time) * 0.28
        $normalized = [Math]::Tanh(($value * 0.22 + $noise) * $Energy)
        $samples[$index] = [int16]([Math]::Round([Math]::Max(-1.0, [Math]::Min(1.0, $normalized)) * 32767.0))
    }

    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create)
    try {
        $writer = [System.IO.BinaryWriter]::new($stream)
        $dataSize = $samples.Length * 2
        $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
        $writer.Write(36 + $dataSize)
        $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVE'))
        $writer.Write([Text.Encoding]::ASCII.GetBytes('fmt '))
        $writer.Write(16)
        $writer.Write([int16]1)
        $writer.Write([int16]1)
        $writer.Write($sampleRate)
        $writer.Write($sampleRate * 2)
        $writer.Write([int16]2)
        $writer.Write([int16]16)
        $writer.Write([Text.Encoding]::ASCII.GetBytes('data'))
        $writer.Write($dataSize)
        foreach ($sample in $samples) { $writer.Write($sample) }
        $writer.Flush()
    }
    finally {
        $stream.Dispose()
    }
}

Write-ImpactWave -Path (Join-Path $OutputDirectory 'RF_Metal_Soft.wav') -Energy 0.36 -Brightness 0.34 -Seed 1701
Write-ImpactWave -Path (Join-Path $OutputDirectory 'RF_Metal_Medium.wav') -Energy 0.66 -Brightness 0.57 -Seed 1702
Write-ImpactWave -Path (Join-Path $OutputDirectory 'RF_Metal_Hard.wav') -Energy 0.94 -Brightness 0.82 -Seed 1703

Get-ChildItem -LiteralPath $OutputDirectory -Filter 'RF_Metal_*.wav' | Select-Object Name, Length, FullName

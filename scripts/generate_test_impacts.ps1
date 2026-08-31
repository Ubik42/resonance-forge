param(
    [string]$OutputDirectory = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Demo\TestAudio\Generated')
)

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function Write-ImpactWave {
    param(
        [string]$Path,
        [double]$Energy,
        [double]$Brightness,
        [int]$Seed,
        [double[]]$Frequencies,
        [double]$DecayBase,
        [double]$DecayStep,
        [double]$NoiseAmount
    )

    $sampleRate = 48000
    $durationSeconds = 1.25
    $sampleCount = [int]($sampleRate * $durationSeconds)
    $samples = New-Object 'System.Int16[]' $sampleCount
    $random = [System.Random]::new($Seed)
    for ($index = 0; $index -lt $sampleCount; $index++) {
        $time = $index / [double]$sampleRate
        $value = 0.0
        for ($mode = 0; $mode -lt $frequencies.Count; $mode++) {
            $tilt = (1.0 - $Brightness) * [Math]::Pow(0.62, $mode) + $Brightness * [Math]::Pow(0.83, $mode)
            $decay = $DecayBase + $mode * $DecayStep
            $phase = $mode * 0.37
            $value += [Math]::Sin(2.0 * [Math]::PI * $frequencies[$mode] * $time + $phase) * [Math]::Exp(-$decay * $time) * $tilt
        }
        $noise = ($random.NextDouble() * 2.0 - 1.0) * [Math]::Exp(-95.0 * $time) * $NoiseAmount
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

$materials = @(
    @{
        Name = 'Steel'
        Frequencies = [double[]]@(224.0, 361.0, 587.0, 927.0, 1438.0, 2215.0, 3390.0)
        DecayBase = 3.2
        DecayStep = 0.72
        NoiseAmount = 0.28
        SeedBase = 1700
    },
    @{
        Name = 'Wood'
        Frequencies = [double[]]@(172.0, 286.0, 451.0, 708.0, 1098.0, 1710.0)
        DecayBase = 5.4
        DecayStep = 0.94
        NoiseAmount = 0.38
        SeedBase = 2700
    },
    @{
        Name = 'Glass'
        Frequencies = [double[]]@(392.0, 615.0, 933.0, 1417.0, 2150.0, 3270.0, 4860.0)
        DecayBase = 2.15
        DecayStep = 0.52
        NoiseAmount = 0.17
        SeedBase = 3700
    }
)

$strikes = @(
    @{ Name = 'Soft'; Energy = 0.36; Brightness = 0.34; SeedOffset = 1 },
    @{ Name = 'Medium'; Energy = 0.66; Brightness = 0.57; SeedOffset = 2 },
    @{ Name = 'Hard'; Energy = 0.94; Brightness = 0.82; SeedOffset = 3 }
)

foreach ($legacyName in @('RF_Metal_Soft.wav', 'RF_Metal_Medium.wav', 'RF_Metal_Hard.wav')) {
    $legacyPath = Join-Path $OutputDirectory $legacyName
    if (Test-Path -LiteralPath $legacyPath) {
        Remove-Item -LiteralPath $legacyPath -Force
    }
}

foreach ($material in $materials) {
    foreach ($strike in $strikes) {
        Write-ImpactWave `
            -Path (Join-Path $OutputDirectory "RF_$($material.Name)_$($strike.Name).wav") `
            -Energy $strike.Energy `
            -Brightness $strike.Brightness `
            -Seed ($material.SeedBase + $strike.SeedOffset) `
            -Frequencies $material.Frequencies `
            -DecayBase $material.DecayBase `
            -DecayStep $material.DecayStep `
            -NoiseAmount $material.NoiseAmount
    }
}

Get-ChildItem -LiteralPath $OutputDirectory -Filter 'RF_*.wav' | Sort-Object Name | Select-Object Name, Length, FullName

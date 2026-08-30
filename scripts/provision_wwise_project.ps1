param(
    [string]$WaapiUrl = 'http://127.0.0.1:8090/waapi',
    [string]$AudioDirectory = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Demo\TestAudio\Generated')
)

$ErrorActionPreference = 'Stop'

function Invoke-Waapi {
    param(
        [string]$Uri,
        [hashtable]$Arguments = @{},
        [hashtable]$Options = @{}
    )
    $body = @{ uri = $Uri; args = $Arguments; options = $Options } | ConvertTo-Json -Depth 20
    Invoke-RestMethod -Method Post -Uri $WaapiUrl -ContentType 'application/json' -Body $body
}

$info = Invoke-Waapi -Uri 'ak.wwise.core.getInfo'

$imports = @(
    @{ audioFile = (Join-Path $AudioDirectory 'RF_Metal_Soft.wav'); objectPath = '\Containers\Default Work Unit\<Random Container>RF_Impact_Metal\<Sound SFX>RF_Metal_Soft' },
    @{ audioFile = (Join-Path $AudioDirectory 'RF_Metal_Medium.wav'); objectPath = '\Containers\Default Work Unit\<Random Container>RF_Impact_Metal\<Sound SFX>RF_Metal_Medium' },
    @{ audioFile = (Join-Path $AudioDirectory 'RF_Metal_Hard.wav'); objectPath = '\Containers\Default Work Unit\<Random Container>RF_Impact_Metal\<Sound SFX>RF_Metal_Hard' }
)

$importResult = Invoke-Waapi -Uri 'ak.wwise.core.audio.import' -Arguments @{
    importOperation = 'useExisting'
    default = @{ importLanguage = 'SFX'; originalsSubFolder = 'ResonanceForge' }
    imports = $imports
} -Options @{ return = @('id', 'name', 'path', 'type') }

$importErrors = @($importResult.log | Where-Object { $_.severity -eq 'Error' })
if ($importErrors.Count -gt 0) {
    throw ($importErrors.message -join [Environment]::NewLine)
}

$eventFolder = Invoke-Waapi -Uri 'ak.wwise.core.object.create' -Arguments @{
    parent = '\Events\Default Work Unit'
    type = 'Folder'
    name = 'ResonanceForge'
    notes = '共振铸造台自动生成的 Wwise 事件。'
    onNameConflict = 'merge'
    children = @(
        @{
            type = 'Event'
            name = 'Play_RF_Impact_Metal'
            notes = '播放金属共振撞击；由 UE 侧 Energy、Brightness、ObjectSize RTPC 驱动。'
            children = @(
                @{ type = 'Action'; name = ''; '@ActionType' = 1; '@Target' = '\Containers\Default Work Unit\RF_Impact_Metal' }
            )
        }
    )
}

$rtpcNames = @('RF_ImpactEnergy', 'RF_ImpactBrightness', 'RF_ObjectSize')
foreach ($rtpcName in $rtpcNames) {
    Invoke-Waapi -Uri 'ak.wwise.core.object.create' -Arguments @{
        parent = '\Game Parameters\Default Work Unit'
        type = 'GameParameter'
        name = $rtpcName
        notes = '共振铸造台 UE 桥接参数；输入范围 0–100。'
        onNameConflict = 'merge'
    } | Out-Null
}

$bank = Invoke-Waapi -Uri 'ak.wwise.core.object.create' -Arguments @{
    parent = '\SoundBanks\Default Work Unit'
    type = 'SoundBank'
    name = 'RF_ResonanceForge'
    notes = '共振铸造台演示 SoundBank。'
    onNameConflict = 'merge'
}

Invoke-Waapi -Uri 'ak.wwise.core.soundbank.setInclusions' -Arguments @{
    soundbank = $bank.id
    operation = 'replace'
    inclusions = @(
        @{ object = '\Events\Default Work Unit\ResonanceForge\Play_RF_Impact_Metal'; filter = @('events', 'structures', 'media') }
    )
} | Out-Null

Invoke-Waapi -Uri 'ak.wwise.core.project.save' | Out-Null

$verification = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
    from = @{ path = @(
        '\Events\Default Work Unit\ResonanceForge\Play_RF_Impact_Metal',
        '\Game Parameters\Default Work Unit\RF_ImpactEnergy',
        '\Game Parameters\Default Work Unit\RF_ImpactBrightness',
        '\Game Parameters\Default Work Unit\RF_ObjectSize',
        '\SoundBanks\Default Work Unit\RF_ResonanceForge'
    ) }
} -Options @{ return = @('id', 'name', 'path', 'type') }

@{
    Wwise = $info.displayName
    ImportedObjectCount = $importResult.objects.Count
    EventFolder = $eventFolder.id
    VerifiedObjects = $verification.return
} | ConvertTo-Json -Depth 10

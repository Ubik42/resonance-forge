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

$materialRoutes = @(
    @{ Key = 'Steel'; DisplayName = '拉丝钢'; Container = 'RF_Impact_Steel'; Event = 'Play_RF_Impact_Steel' },
    @{ Key = 'Wood'; DisplayName = '硬木'; Container = 'RF_Impact_Wood'; Event = 'Play_RF_Impact_Wood' },
    @{ Key = 'Glass'; DisplayName = '薄玻璃'; Container = 'RF_Impact_Glass'; Event = 'Play_RF_Impact_Glass' }
)
$strikeNames = @('Soft', 'Medium', 'Hard')

$imports = @($materialRoutes | ForEach-Object {
    $route = $_
    $strikeNames | ForEach-Object {
        $soundName = "RF_$($route.Key)_$_"
        @{
            audioFile = (Join-Path $AudioDirectory "$soundName.wav")
            objectPath = "\Containers\Default Work Unit\<Random Container>$($route.Container)\<Sound SFX>$soundName"
            soundPath = "\Containers\Default Work Unit\$($route.Container)\$soundName"
        }
    }
})

$missingAudio = @($imports | Where-Object { -not (Test-Path -LiteralPath $_.audioFile) })
if ($missingAudio.Count -gt 0) {
    throw "缺少材质撞击测试音频：$($missingAudio.audioFile -join '、')。请先运行 scripts/generate_test_impacts.ps1。"
}

$pendingImports = @($imports | Where-Object {
    try {
        $existing = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
            from = @{ path = @($_.soundPath) }
        } -Options @{ return = @('id') }
        @($existing.return).Count -eq 0
    }
    catch {
        $true
    }
})

$importResult = @{ objects = @(); log = @() }
if ($pendingImports.Count -gt 0) {
    $importResult = Invoke-Waapi -Uri 'ak.wwise.core.audio.import' -Arguments @{
        importOperation = 'useExisting'
        default = @{ importLanguage = 'SFX'; originalsSubFolder = 'ResonanceForge' }
        imports = @($pendingImports | ForEach-Object {
            @{ audioFile = $_.audioFile; objectPath = $_.objectPath }
        })
    } -Options @{ return = @('id', 'name', 'path', 'type') }
}

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
    children = @($materialRoutes | ForEach-Object {
        @{
            type = 'Event'
            name = $_.Event
            notes = "播放$($_.DisplayName)撞击；由 UE 侧材质路由和 Energy、Brightness、ObjectSize RTPC 驱动。"
            children = @(
                @{ type = 'Action'; name = ''; '@ActionType' = 1; '@Target' = "\Containers\Default Work Unit\$($_.Container)" }
            )
        }
    })
}

$rtpcNames = @('RF_ImpactEnergy', 'RF_ImpactBrightness', 'RF_ObjectSize')
$rtpcIds = @{}
foreach ($rtpcName in $rtpcNames) {
    $rtpcResult = Invoke-Waapi -Uri 'ak.wwise.core.object.create' -Arguments @{
        parent = '\Game Parameters\Default Work Unit'
        type = 'GameParameter'
        name = $rtpcName
        notes = '共振铸造台 UE 桥接参数；输入范围 0–100。'
        onNameConflict = 'merge'
        '@Min' = 0
        '@Max' = 100
        '@InitialValue' = 50
        '@SimulationValue' = 50
    }
    $rtpcIds[$rtpcName] = $rtpcResult.id
}

$containerPaths = @($materialRoutes | ForEach-Object { "\Containers\Default Work Unit\$($_.Container)" })
$rtpcMappings = @(
    @{
        Parameter = 'RF_ImpactEnergy'
        Property = 'Volume'
        Description = '撞击能量控制响度：弱撞击保留可听底噪，强撞击到达标称电平。'
        Points = @(
            @{ x = 0; y = -24; shape = 'Exp1' },
            @{ x = 20; y = -12; shape = 'SCurve' },
            @{ x = 55; y = -4; shape = 'Log1' },
            @{ x = 100; y = 0; shape = 'Linear' }
        )
    },
    @{
        Parameter = 'RF_ImpactBrightness'
        Property = 'Lowpass'
        Description = '明亮度反向控制 Voice LPF：数值越高，低通越少。'
        Points = @(
            @{ x = 0; y = 82; shape = 'Log3' },
            @{ x = 45; y = 34; shape = 'SCurve' },
            @{ x = 100; y = 0; shape = 'Linear' }
        )
    },
    @{
        Parameter = 'RF_ObjectSize'
        Property = 'Pitch'
        Description = '共振尺度控制音高：小物体略升高，大物体降低。'
        Points = @(
            @{ x = 0; y = 420; shape = 'SCurve' },
            @{ x = 50; y = 0; shape = 'SCurve' },
            @{ x = 100; y = -520; shape = 'Linear' }
        )
    }
)

$rtpcObjects = @($rtpcMappings | ForEach-Object {
    @{
        type = 'RTPC'
        name = ''
        notes = $_.Description
        '@PropertyName' = $_.Property
        '@ControlInput' = $rtpcIds[$_.Parameter]
        '@Curve' = @{
            type = 'Curve'
            points = $_.Points
        }
    }
})

function Test-RtpcMappings {
    param([object[]]$ActualMappings)

    if ($ActualMappings.Count -ne $rtpcMappings.Count) {
        return $false
    }
    foreach ($expected in $rtpcMappings) {
        $actual = @($ActualMappings | Where-Object { $_.'@PropertyName' -eq $expected.Property })
        if ($actual.Count -ne 1 -or $actual[0].'@ControlInput'.id -ne $rtpcIds[$expected.Parameter]) {
            return $false
        }
        $actualPoints = @($actual[0].'@Curve'.points)
        if ($actualPoints.Count -ne $expected.Points.Count) {
            return $false
        }
        for ($index = 0; $index -lt $expected.Points.Count; $index++) {
            $expectedPoint = $expected.Points[$index]
            $actualPoint = $actualPoints[$index]
            if ([Math]::Abs([double]$actualPoint.x - [double]$expectedPoint.x) -gt 0.0001 -or
                [Math]::Abs([double]$actualPoint.y - [double]$expectedPoint.y) -gt 0.0001 -or
                [string]$actualPoint.shape -ne [string]$expectedPoint.shape) {
                return $false
            }
        }
    }
    return $true
}

function Get-CurrentRtpcMappings {
    param([string]$ContainerPath)

    $containerResult = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
        from = @{ path = @($ContainerPath) }
    } -Options @{ return = @('id', '@RTPC') }
    $mappingIds = @($containerResult.return[0].'@RTPC' | Where-Object { $_ -and $_.id } | ForEach-Object { $_.id })
    if ($mappingIds.Count -eq 0) {
        return @()
    }

    $mappingDetails = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
        from = @{ id = $mappingIds }
    } -Options @{ return = @('id', 'name', 'type', '@PropertyName', '@ControlInput', '@Curve') }

    foreach ($mapping in @($mappingDetails.return)) {
        $curveId = $mapping.'@Curve'.id
        if ($curveId) {
            $curveResult = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
                from = @{ id = @($curveId) }
            } -Options @{ return = @('id', 'points') }
            $mapping.'@Curve' | Add-Member -NotePropertyName points -NotePropertyValue @($curveResult.return[0].points) -Force
        }
    }
    return @($mappingDetails.return)
}

$configuredContainers = @()
foreach ($containerPath in $containerPaths) {
    $currentMappings = @(Get-CurrentRtpcMappings -ContainerPath $containerPath)

    if (Test-RtpcMappings -ActualMappings $currentMappings) {
        $createdMappings = $currentMappings
    }
    else {
        $mappingResult = Invoke-Waapi -Uri 'ak.wwise.core.object.set' -Arguments @{
            objects = @(
                @{
                    object = $containerPath
                    '@RTPC' = $rtpcObjects
                }
            )
            onNameConflict = 'merge'
            listMode = 'replaceAll'
        } -Options @{ return = @('id', 'name', 'type', '@PropertyName', '@ControlInput', '@Curve') }
        $createdMappings = @($mappingResult.objects[0].'@RTPC')
    }

    if ($createdMappings.Count -ne $rtpcMappings.Count) {
        throw "$containerPath 的 RTPC 映射数量不正确：期望 $($rtpcMappings.Count)，实际 $($createdMappings.Count)。"
    }
    foreach ($expected in $rtpcMappings) {
        $actual = @($createdMappings | Where-Object { $_.'@PropertyName' -eq $expected.Property })
        if ($actual.Count -ne 1) {
            throw "$containerPath 的 RTPC 属性映射缺失或重复：$($expected.Property)。"
        }
        if ($actual[0].'@ControlInput'.id -ne $rtpcIds[$expected.Parameter]) {
            throw "$containerPath 的 RTPC 控制输入不正确：$($expected.Parameter) → $($expected.Property)。"
        }
        if (@($actual[0].'@Curve'.points).Count -ne $expected.Points.Count) {
            throw "$containerPath 的 RTPC 曲线控制点不完整：$($expected.Property)。"
        }
    }
    $configuredContainers += $containerPath
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
    inclusions = @($materialRoutes | ForEach-Object {
        @{ object = "\Events\Default Work Unit\ResonanceForge\$($_.Event)"; filter = @('events', 'structures', 'media') }
    })
} | Out-Null

$legacyPaths = @(
    '\Events\Default Work Unit\ResonanceForge\Play_RF_Impact_Metal',
    '\Containers\Default Work Unit\RF_Impact_Metal'
)
foreach ($legacyPath in $legacyPaths) {
    $legacy = @{ return = @() }
    try {
        $legacy = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
            from = @{ path = @($legacyPath) }
        } -Options @{ return = @('id') }
    }
    catch {
        $legacy = @{ return = @() }
    }
    if (@($legacy.return).Count -gt 0) {
        Invoke-Waapi -Uri 'ak.wwise.core.object.delete' -Arguments @{ object = $legacyPath } | Out-Null
    }
}

Invoke-Waapi -Uri 'ak.wwise.core.project.save' | Out-Null

$verificationPaths = @($materialRoutes | ForEach-Object {
    "\Events\Default Work Unit\ResonanceForge\$($_.Event)"
}) + @(
    '\Game Parameters\Default Work Unit\RF_ImpactEnergy',
    '\Game Parameters\Default Work Unit\RF_ImpactBrightness',
    '\Game Parameters\Default Work Unit\RF_ObjectSize',
    '\SoundBanks\Default Work Unit\RF_ResonanceForge'
)
$verification = Invoke-Waapi -Uri 'ak.wwise.core.object.get' -Arguments @{
    from = @{ path = $verificationPaths }
} -Options @{ return = @('id', 'name', 'path', 'type') }

if (@($verification.return).Count -ne $verificationPaths.Count) {
    throw "Wwise 材质路由验证不完整：期望 $($verificationPaths.Count) 个对象，实际 $(@($verification.return).Count) 个。"
}

@{
    Wwise = $info.displayName
    ImportedObjectCount = $importResult.objects.Count
    EventFolder = $eventFolder.id
    MaterialRoutes = $materialRoutes
    ConfiguredContainers = $configuredContainers
    RtpcMappings = @($rtpcMappings | ForEach-Object {
        @{
            Parameter = $_.Parameter
            Property = $_.Property
            Points = $_.Points
        }
    })
    VerifiedObjects = $verification.return
} | ConvertTo-Json -Depth 10

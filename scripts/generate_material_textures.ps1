param(
    [string]$OutputDirectory = "D:\3D\_tools\resonance-forge\Demo\TestMaterials\Generated"
)

Add-Type -AssemblyName System.Drawing
$target = [System.IO.Path]::GetFullPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($target) | Out-Null
$size = 1024
$rng = [System.Random]::new(20260829)

function New-Canvas([System.Drawing.Color]$color) {
    $bitmap = [System.Drawing.Bitmap]::new($size, $size)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear($color)
    return @($bitmap, $graphics)
}

function Save-Canvas($canvas, [string]$name) {
    $bitmap = $canvas[0]
    $graphics = $canvas[1]
    $graphics.Dispose()
    $path = Join-Path $target $name
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

# 拉丝钢：冷灰渐变、单向拉丝与细微划痕。
$steel = New-Canvas ([System.Drawing.Color]::FromArgb(255, 61, 79, 95))
for ($y = 0; $y -lt $size; $y += 2) {
    $shade = 70 + [int](22 * [Math]::Sin($y * 0.031)) + $rng.Next(-10, 11)
    $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(145, $shade, [Math]::Min(255, $shade + 17), [Math]::Min(255, $shade + 28)), 1)
    $steel[1].DrawLine($pen, 0, $y, $size, $y)
    $pen.Dispose()
}
for ($i = 0; $i -lt 180; $i++) {
    $y = $rng.Next(0, $size)
    $x = $rng.Next(0, $size - 120)
    $length = $rng.Next(45, 420)
    $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb($rng.Next(22, 65), 205, 232, 248), 1)
    $steel[1].DrawLine($pen, $x, $y, [Math]::Min($size, $x + $length), $y)
    $pen.Dispose()
}
Save-Canvas $steel "T_RF_Steel_BaseColor.png"

$steelR = New-Canvas ([System.Drawing.Color]::FromArgb(255, 58, 58, 58))
for ($y = 0; $y -lt $size; $y += 3) {
    $value = $rng.Next(34, 88)
    $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, $value, $value, $value), 1)
    $steelR[1].DrawLine($pen, 0, $y, $size, $y)
    $pen.Dispose()
}
Save-Canvas $steelR "T_RF_Steel_Roughness.png"

# 硬木：暖棕底色、年轮波纹和深色木结。
$wood = New-Canvas ([System.Drawing.Color]::FromArgb(255, 104, 43, 17))
for ($y = 0; $y -lt $size; $y += 5) {
    $wave = [int](13 * [Math]::Sin($y * 0.046) + 7 * [Math]::Sin($y * 0.013))
    $r = [Math]::Max(45, [Math]::Min(175, 118 + $wave + $rng.Next(-9, 10)))
    $g = [Math]::Max(22, [Math]::Min(105, 58 + [int]($wave * 0.45)))
    $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(210, $r, $g, 24), $rng.Next(2, 6))
    $wood[1].DrawBezier($pen, -40, $y, 280, $y + $rng.Next(-28, 29), 720, $y + $rng.Next(-28, 29), 1080, $y)
    $pen.Dispose()
}
for ($i = 0; $i -lt 14; $i++) {
    $x = $rng.Next(70, $size - 70); $y = $rng.Next(70, $size - 70)
    for ($ring = 4; $ring -ge 1; $ring--) {
        $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(150, 47, 19, 8), 3)
        $wood[1].DrawEllipse($pen, $x - 18 * $ring, $y - 8 * $ring, 36 * $ring, 16 * $ring)
        $pen.Dispose()
    }
}
Save-Canvas $wood "T_RF_Wood_BaseColor.png"

$woodR = New-Canvas ([System.Drawing.Color]::FromArgb(255, 144, 144, 144))
for ($y = 0; $y -lt $size; $y += 5) {
    $value = $rng.Next(118, 185)
    $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, $value, $value, $value), 3)
    $woodR[1].DrawBezier($pen, 0, $y, 260, $y - 18, 760, $y + 24, $size, $y)
    $pen.Dispose()
}
Save-Canvas $woodR "T_RF_Wood_Roughness.png"

# 薄玻璃：青蓝磨砂层、边缘高光和放射状细裂纹。
$glass = New-Canvas ([System.Drawing.Color]::FromArgb(255, 13, 83, 96))
for ($i = 0; $i -lt 900; $i++) {
    $x = $rng.Next(0, $size); $y = $rng.Next(0, $size); $radius = $rng.Next(1, 8)
    $brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb($rng.Next(12, 42), 125, 244, 236))
    $glass[1].FillEllipse($brush, $x, $y, $radius, $radius)
    $brush.Dispose()
}
for ($origin = 0; $origin -lt 7; $origin++) {
    $cx = $rng.Next(150, $size - 150); $cy = $rng.Next(150, $size - 150)
    for ($branch = 0; $branch -lt 6; $branch++) {
        $angle = ($branch / 6.0) * [Math]::PI * 2 + $rng.NextDouble() * 0.45
        $length = $rng.Next(70, 230)
        $ex = $cx + [Math]::Cos($angle) * $length; $ey = $cy + [Math]::Sin($angle) * $length
        $pen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(118, 180, 255, 250), 2)
        $glass[1].DrawLine($pen, $cx, $cy, [int]$ex, [int]$ey)
        $pen.Dispose()
    }
}
Save-Canvas $glass "T_RF_Glass_BaseColor.png"

$glassR = New-Canvas ([System.Drawing.Color]::FromArgb(255, 72, 72, 72))
for ($i = 0; $i -lt 700; $i++) {
    $value = $rng.Next(45, 125)
    $brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(90, $value, $value, $value))
    $glassR[1].FillEllipse($brush, $rng.Next(0, $size), $rng.Next(0, $size), $rng.Next(2, 10), $rng.Next(2, 10))
    $brush.Dispose()
}
Save-Canvas $glassR "T_RF_Glass_Roughness.png"

Get-ChildItem -LiteralPath $target -Filter 'T_RF_*.png' | Select-Object Name, Length, FullName

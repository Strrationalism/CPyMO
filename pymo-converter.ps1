
$device_specs = @(
    @{ Name = "s60v3"; Width = 320; Height = 240; UseMask = $true; Movie = $true;
       BGFormat = "jpg"; Charaformat = "jpg"; PlatformId = "s60v3"; Audio = @("mp3", "ogg", "wav"); },

    @{ Name = "s60v5"; Width = 540; Height = 360; UseMask = $true; Movie = $true;
       BGFormat = "jpg"; Charaformat = "jpg"; PlatformId = "s60v5"; Audio = @("mp3", "ogg", "wav"); },

    @{ Name = "3ds"; Width = 400; Height = 240; UseMask = $false; Movie = $true;
       BGFormat = "jpg"; Charaformat = "png"; PlatformId = "pygame"; Audio = @("mp3", "ogg", "wav") },

    @{ Name = "pymo-android"; Width = 800; Height = 600; UseMask = $false; Movie = $true;
       BGFormat = "png"; Charaformat = "png"; PlatformId = "pygame"; Audio = @("ogg", "wav") },

    @{ Name = "switch"; Width = 1920; Height = 1080; UseMask = $false; Movie = $true;
       BGFormat = "png"; Charaformat = "png"; PlatformId = "pygame"; Audio = @("mp3", "ogg", "wav") },

    @{ Name = "psp"; Width = 480; Height = 272; UseMask = $true; Movie = $false;
       BGFormat = "jpg"; Charaformat = "jpg"; PlatformId = "s60v3"; Audio = @("ogg", "wav") },

    @{ Name = "psv"; Width = 960; Height = 544; UseMask = $false; Movie = $false;
       BGFormat = "png"; Charaformat = "png"; PlatformId = "pygame"; Audio = @("ogg", "wav") }
)

function Write-Help() {
    Write-Host "PyMO Game Converter"
    Write-Host ""
    Write-Host "You must ensure cpymo-tool has installed!"
    Write-Host ""
    Write-Host "Usage:"
	Write-Host "    pymo-convert.ps1 <device-spec> <src-game> <dst-dir>"
    Write-Host ""
    Write-Host "Avaliable device specs:"
    foreach ($i in $device_specs) {
        Write-Host "    - $($i.Name)"
    }
}

function Ensure-Dir($dir) {
    if (-not (Test-Path $dir)) {
        md $dir
    }
}

if ($args.Length -ne 3) { 
    Write-Help
    Break Script
}


$spec = $null

foreach ($i in $device_specs) {
    if ($i.Name -eq $args[0]) {
        $spec = $i
        Break
    }
}

if ($spec -eq $null) {
    Write-Host "[Error] Can not get device spec $($args[0])."
    Write-Host ""
    Write-Help
}

function Parse-GameConfig($filename) {
    $src = Get-Content $filename
    $dst = [Ordered]@{}
    foreach ($line in $src) {
        $line = $line.Trim()
        $p = $line.IndexOf(',')
        if ($p -ge 0) {
            $key = $line.Substring(0, $p)
            $value = $line.Substring($p + 1)
            $dst[$key] = $value
        }
    }
    return $dst
}

$gamedir = $args[1]
$outdir = $args[2]
Ensure-Dir $outdir

if (-not (Test-Path "$gamedir/gameconfig.txt")) {
    Write-Host "Can not find gameconfig.txt"
    Break Script
}

$gameconfig = Parse-GameConfig "$gamedir/gameconfig.txt"
$src_width = [int]($gameconfig["imagesize"].Split(',')[0])
$src_height = [int]($gameconfig["imagesize"].Split(',')[1])
$ratio = [math]::Min([double]$spec.Width / [double]$src_width, [double]$spec.Height / [double]$src_height)

$cpymo_tool_transparent_arg = @()

if (($gameconfig["platform"] -eq "s60v3") -or ($gameconfig["platform"] -eq "s60v5")) {
    $cpymo_tool_transparent_arg += "--load-mask"
}

if ($spec.UseMask) {
    $cpymo_tool_transparent_arg += "--create-mask"
}

function Transparent-Filter($asstype, $name) {
    if ($asstype -eq "bg") { return $false }
    if ($asstype -eq "system") {
        if ($name.StartsWith("albumbg_")) { return $false }
        if ($name.StartsWith("cvThumb")) { return $false }
    }

    return $true
}

function Convert-Images($asstype, $src_format, $dst_format, $src_dir, $dst_dir) {
    Ensure-Dir $dst_dir

    ls "$src_dir/*.$src_format" | ForEach-Object {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($_.FullName)
        $dst_name = "$dst_dir/$name.$dst_format"

        if (-not (Transparent-Filter $asstype $name)) {
            cpymo-tool resize "$($_.FullName)" "$($dst_name)" $ratio $ratio
        }
        else {
            cpymo-tool resize "$($_.FullName)" "$($dst_name)" $ratio $ratio $cpymo_tool_transparent_arg
        }

        
        Write-Host "Resize $asstype/$name"
    }
}

function Unpack-Convert-Pack($asstype, $src_format, $dst_format, $src_dir, $dst_dir, $unpack_dir, $convert_dir) {
    Ensure-Dir $convert_dir

    if (Test-Path "$src_dir/$asstype.pak") {
        Ensure-Dir $unpack_dir
        cpymo-tool unpack "$src_dir/$asstype.pak" ".$src_format" "$unpack_dir"
        Convert-Images $asstype $src_format $dst_format $unpack_dir $convert_dir
        Remove-Item $unpack_dir -Recurse -Force
    } else {
        Convert-Images $asstype $src_format $dst_format $src_dir $convert_dir
    }
    
    Ensure-Dir $dst_dir
    $dst_dir_full = [System.IO.Path]::GetFullPath($dst_dir)
    pushd $convert_dir

    $files = @()
    ls "*" | ForEach-Object {
        $files += $_.Name
    }

    Out-File `
        -FilePath "list.txt" `
        -Force `
        -InputObject $files `
        -Encoding utf8

    cpymo-tool pack "$dst_dir_full/$asstype.pak" --file-list "./list.txt"
    popd
    Remove-Item $convert_dir -Recurse -Force
}


Unpack-Convert-Pack `
    "bg" `
    ($gameconfig["bgformat"].TrimStart('.')) `
    ($spec.BGFormat) `
    "$gamedir/bg" `
    "$outdir/bg" `
    "$outdir/bg-unpack" `
    "$outdir/bg-convert"


Unpack-Convert-Pack `
    "chara" `
    ($gameconfig["charaformat"].TrimStart('.')) `
    ($spec.Charaformat) `
    "$gamedir/chara" `
    "$outdir/chara" `
    "$outdir/chara-unpack" `
    "$outdir/chara-convert"


Convert-Images "system" "png" "png" "$gamedir/system" "$outdir/system"

if ($spec.Movie -and (Test-Path "$gamedir/video") -and ($gameconfig["playvideo"].Trim() -eq "1")) {
    $gameconfig["playvideo"] = 1

    md "$outdir/video"

    ls "$gamedir/video" | ForEach-Object {
        Write-Host $_.Name
        ffmpeg -i $_.FullName -vf scale="iw * $ratio : ih * $ratio" -c:v mpeg4 -c:a aac "$outdir/video/$($_.Name)"
    }
}
else {
    $gameconfig["playvideo"] = 0
}

@("se", "voice", "icon.png", "bgm", "script") | ForEach-Object {
    Write-Host "Coping $_..."
    if (Test-Path "$gamedir/$_") {
        Copy-Item "$gamedir/$_" "$outdir/$_" -Recurse -Force
    }
}


$gameconfig["imagesize"] = "$([int]($src_width * $ratio)),$([int]($src_height * $ratio))"
$gameconfig["bgformat"] = ".$($spec.BGFormat)"
$gameconfig["charaformat"] = ".$($spec.Charaformat)"
$gameconfig["platform"] = $spec.PlatformId
$gameconfig["charamaskformat"] = $gameconfig["charaformat"]

$gameconfig_lines = @()
foreach ($gameconfig_k in $gameconfig.Keys) {
    $gameconfig_lines += "$gameconfig_k,$($gameconfig[$gameconfig_k])"
}

Out-File `
    -FilePath "$outdir/gameconfig.txt" `
    -Force `
    -InputObject $gameconfig_lines `
    -Encoding utf8

$bgmformat_supported = $spec.Audio.Contains($gameconfig["bgmformat"].Trim().TrimStart('.').Trim())
$seformat_supported = $spec.Audio.Contains($gameconfig["seformat"].Trim().TrimStart('.').Trim())
$voformat_supported = $spec.Audio.Contains($gameconfig["voiceformat"].Trim().TrimStart('.').Trim())

if ((-not $bgmformat_supported) -or (-not $seformat_supported) -or (-not $voformat_supported)) {
    pymo-convert-audio-to-ogg.ps1 $outdir
    
    if (Test-Path "$outdir/bgm-backup") { Delete-Item -Recurse -Force "$outdir/bgm-backup" }
    if (Test-Path "$outdir/se-backup") { Delete-Item -Recurse -Force "$outdir/se-backup" }
    if (Test-Path "$outdir/voice-backup") { Delete-Item -Recurse -Force "$outdir/voice-backup" }
    if (Test-Path "$outdir/gameconfig-backup.txt") { Delete-Item -Force "$outdir/gameconfig-backup.txt" }
}

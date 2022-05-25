function Write-Help() {
    Write-Host "Strip PyMO Game"
    Write-Host ""
    Write-Host "You must ensure cpymo-tool is installed!"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "    pymo-strip.ps1 <gamedir>"
    Write-Host ""
}

if ($args.Length -ne 1) {
    Write-Help
    Break Script
}

$script:dir = "$pwd/$($args[0])"

if (Test-Path "$dir/strip") {
    Write-Host "Strip directory already exists, you must delete to continue."
    Break Script
}

mkdir "$dir/strip"

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

$script:gc = Parse-GameConfig "$dir/gameconfig.txt"

$script:asstypes = @(
    @("bg", $gc["bgformat"]), 
    @("bgm", $gc["bgmformat"]), 
    @("chara", $gc["charaformat"]), 
    @("script", ".txt"), 
    @("se", $gc["seformat"]), 
    @("voice", $gc["voiceformat"]), 
    @("video", ".mp4"), 
    @("system", ".png"))

$script:packed_assets = @()
$script:use_mask = ($gc["platform"] -eq "s60v3") -or ($gc["platform"] -eq "s60v5")

foreach ($i in $asstypes) {
    $asstype = $i[0]
    $ext = $i[1]
    if (Test-Path "$dir/$asstype") {
        if (Test-Path "$dir/$asstype/$asstype.pak") {
            $_ = mkdir "$dir/strip/$asstype"
            cpymo-tool unpack "$dir/$asstype/$asstype.pak" $ext "$dir/strip/$asstype"
            rm -Recurse -Force "$dir/$asstype"
            $packed_assets += $asstype
        } else {
            Move-Item "$dir/$asstype" "$dir/strip/$asstype" -Force
        }
    }
}

$script:actived_asset = New-Object System.Collections.Generic.HashSet[String]

function Active-Asset($asstype, $filename, $ext, $mask, $mask_format, $warn = $true) {
    $dst = "$dir/$asstype/$filename$ext"
    if (-not $actived_asset.Contains($dst.ToLower())) {
        if (-not (Test-Path "$dir/$asstype")) {
            $_ = mkdir "$dir/$asstype"
        }

        $src = "$dir/strip/$asstype/$filename$ext"
        

        if (-not (Test-Path $src)) {
            Write-Host ("[Warning] Can not find file $dst.")
            return
        } else {
            Move-Item $src $dst -Force
        }

        if ($mask -and $use_mask) {
            Active-Asset $asstype "$($filename)_mask" $mask_format $false "" $false
            Active-Asset $asstype "$($filename)_mask" $ext $false "" $false
        }

        $_ = $actived_asset.Add($dst.ToLower())
    }
}

function Parse-Script($line) {
    $p = $line.IndexOf(' ')
    if ($p -ge 0) {
        $x = $line.Substring(0, $p).Trim()
        $args = $line.Substring($p).Trim().Split(',')
        $ret = @{ Cmd = $x; Args = @() }
        foreach ($a in $args) {
            $ret.Args += $a.Trim()
        }
        return $ret
    } else {
        return @{ Cmd = $line; Args = @() }
    }
}

$script:actived_script = New-Object System.Collections.Generic.HashSet[String]
function Active-Script($script_name) {
    if ($actived_script.Contains($script_name.ToLower())) {
        return
    }

    $_ = $actived_script.Add($script_name.ToLower())

    Write-Host "Processing $script_name..."
    Active-Asset "script" $script_name ".txt" $false ""

    $content = [System.IO.File]::ReadAllLines("$dir/script/$script_name.txt")
    foreach ($line in $content) {
        $line = $line.Trim()
        if ($line.StartsWith("#")) {
            $cmd = Parse-Script $line
            $a = $cmd.Args
            switch ($cmd.Cmd.TrimStart('#')) {
                "say" {
                    Active-Asset "system" "menu" ".png" $true ".png"
                    Active-Asset "system" "sel_highlight" ".png" $true ".png"
                    Active-Asset "system" "message" ".png" $true ".png"
                    Active-Asset "system" "name" ".png" $true ".png"
                    Active-Asset "system" "message_cursor" ".png" $true ".png"
                    break
                }
                "text" {
                    Active-Asset "system" "menu" ".png" $true ".png"
                    Active-Asset "system" "message_cursor" ".png" $true ".png"
                    break
                }
                "chara" {
                    for ($i = 0; $i -lt $a.Length; $i += 4) {
                        if ($i + 4 -gt $a.Length) { break }
                        Active-Asset "chara" $a[$i + 1] $gc["charaformat"] $true $gc["charamaskformat"]
                    }
    
                    break
                }
                "bg" {
                    Active-Asset "bg" $a[0] $gc["bgformat"] $false ""
                    if ($a.Length -ge 2) {
                        if (($a[1] -ne "BG_ALPHA") -and
                            ($a[1] -ne "BG_FADE") -and
                            ($a[1] -ne "BG_NOFADE")) {

                            Active-Asset "system" $a[1] ".png" $false ""
                        }
                    }
                    break
                }
                "movie" {
                    $a = $a[0]
                    Active-Asset "video" $a ".mp4" $false ""
                    break
                }
                "textbox" {
                    Active-Asset "system" $a[0] ".png" $true ".png"
                    Active-Asset "system" $a[1] ".png" $true ".png"
                    break
                }
                "scroll" {
                    Active-Asset "bg" $a[0] $gc["bgformat"] $false ""
                    break
                }
                "chara_y" {
                    $i = 0
                    while ($i + 5 -lt $a.Length) {
                        Active-Asset "chara" $a[$i + 2] $gc["charaformat"] $true $gc["charamaskformat"]
                        $i += 5
                    }
                }
                "chara_scroll" {
                    Active-Asset "chara" $a[2] $gc["charaformat"] $true $gc["charamaskformat"]
                    break
                }
                "anime_on" {
                    Active-Asset "system" $a[1] ".png" $true ".png"
                    break
                }
                "change" {
                    Active-Script $a[0]
                    break
                }
                "call" {
                    Active-Script $a[0]
                    break
                }
                "sel" {
                    Active-Asset "system" "sel_highlight" ".png" $true ".png"
                    Active-Asset "system" "option" ".png" $true ".png"
                    Active-Asset "system" "menu" ".png" $true ".png"
                    break
                }
                "select_text" {
                    Active-Asset "system" "sel_highlight" ".png" $true ".png"
                    
                    if ($a[$a.Lenght - 1] -eq "-1") {
                        Active-Asset "system" "menu" ".png" $true ".png"
                    }

                    break
                }
                "select_var" {
                    Active-Asset "system" "sel_highlight" ".png" $true ".png"

                    if ($a[$a.Lenght - 1] -eq "-1") {
                        Active-Asset "system" "menu" ".png" $true ".png"
                    }

                    break
                }
                "select_img" {
                    if ($a[$a.Lenght - 1] -eq "-1") {
                        Active-Asset "system" "menu" ".png" $true ".png"
                    }

                    Active-Asset "system" $a[1] ".png" $true ".png"
                    break
                }

                # select_imgs
                "bgm" {
                    Active-Asset "bgm" $a[0] $gc["bgmformat"] $false ""
                    break
                }
                "se" {
                    Active-Asset "se" $a[0] $gc["seformat"] $false ""
                    break
                }
                "vo" {
                    Active-Asset "voice" $a[0] $gc["voiceformat"] $false ""
                    break
                }
                "album" {
                    Active-Asset "system" "cvThumb" ".png" $false ""
                    $album_png = "albumbg"
                    if ($a.Length -eq 0) {
                        Active-Asset "script" "album_list" ".txt" $false ""
                    } else {
                        if ([System.String]::IsWhitespaceOrNull($a[0])) {
                            Active-Asset "script" "album_list" ".txt" $false ""
                        } else {
                            Active-Asset "script" $a[0] ".txt" $false ""
                            $album_png = $a[0]
                        }
                    }

                    $i = 0
                    while (Test-Path "$dir/strip/system/$($album_png)_$i.png") {
                        Active-Asset "system" "$($album_png)_$i" ".png" $false "" $false
                        $i += 1
                    }

                    if ($i -eq 0) {
                        Active-Asset "system" $album_png ".png" $false ""
                    }

                    break
                }
                "music" {
                    Active-Asset "script" "music_list" ".txt" $false ""
                    break
                }
                "date" {
                    Active-Asset "system" $a[0] ".png" $false ""
                    break
                }
            }
        }
    }
}


Active-Asset "bg" "logo1" $gc["bgformat"] $false ""
Active-Asset "bg" "logo2" $gc["bgformat"] $false ""
Active-Asset "system" "menu" ".png" $true ".png"
Active-Script $gc["startscript"]


function Pack-Dir($asstype) {
    pushd "$dir/$asstype"
    $files = @()
    ls "*" | ForEach-Object {
        $files += $_.Name
    }

    Out-File `
        -FilePath "./list.txt" `
        -Force `
        -InputObject $files `
        -Encoding utf8

    cpymo-tool pack "$asstype.pak" --file-list "./list.txt"
    rm -Force "./list.txt"

    foreach ($i in $files) {
        rm $i -Force
    }

    popd
}

foreach ($ass in $packed_assets) {
    Pack-Dir $ass
}

rm -Recurse -Force "$dir/strip"

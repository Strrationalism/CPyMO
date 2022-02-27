
# 将此代码复制粘贴到记事本，保存到目标PyMO游戏目录下命名为"mo2pymo.ps1"
# 右键点击“使用PowerShell”执行即可

function Write-Help {
    Write-Host "此补丁用于将使用MO1和MO2脚本编写的PyMO游戏转换为PyMO脚本。"
    Write-Host "此补丁仅用于以下PyMO游戏："
    Write-Host "  ·缘之空"
    Write-Host "  ·秋之回忆"
    Write-Host "  ·秋之回忆2"
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

function Verify-GameConfig($gameconfig) {
    if (($gameconfig["gametitle"] -ne "缘之空\n制作发行：Sphere") -and
        ($gameconfig["gametitle"] -ne "缘之空\n制作发行： Sphere") -and
        ($gameconfig["gametitle"] -ne "秋之回忆1\nAndroid") -and
        ($gameconfig["gametitle"] -ne "秋之回忆1\nS60v3") -and
        ($gameconfig["gametitle"] -ne "秋之回忆1\nS60v5") -and
        ($gameconfig["gametitle"] -ne "秋之回忆2\n汉化：CK-GAL中文化小组\n移植：chen_xin_ming") -and
        ($gameconfig["gametitle"] -ne "秋之回忆2\n移植：chen_xin_ming\n汉化：CK-GAL中文化小组"))
    {
        Write-Host "目标游戏不受支持。"
        Write-Help
        Break Script
    }

    if ($gameconfig["scripttype"] -eq "pymo") {
        Write-Host "目标游戏已经应用了此补丁。"
        Break Script
    }

    if (($gameconfig["scripttype"] -ne "mo1") -and 
        ($gameconfig["scripttype"] -ne "mo2"))
    {
        Write-Host "目标游戏脚本类型为$($game_config["scripttype"])，不受支持。"
        Write-Help
        Break Script
    }
}

function Process-GameConfig {
    if (Test-Path "gameconfig.txt") {
        $gameconfig = Parse-GameConfig "gameconfig.txt"
        Verify-GameConfig $gameconfig
        Copy-Item "gameconfig.txt" -Destination "gameconfig-bak.txt"
        $is_mo1 = $gameconfig["scripttype"] -eq "mo1"
        $gameconfig["scripttype"] = "pymo"

        $lines = @()
        foreach($gameconfig_k in $gameconfig.Keys) {
            $lines += "$gameconfig_k,$($gameconfig[$gameconfig_k])"
        }

        Out-File `
            -FilePath "gameconfig.txt" `
            -Force `
            -InputObject $lines `
            -Encoding utf8

        return $is_mo1

    } else {
        Write-Host "找不到gameconfig.txt"
        Write-Help
        Break Script
    }
}

$non_script = @(
    "music_list.txt",
    "album_list.txt",
    "omake_list.txt",
    "song_titles.txt",
    "title_list.txt"
)

function Get-ScriptList($dir) {
    return (Get-ChildItem $dir -Exclude $non_script)
}

$wtf_symbols_mo1 = @(
    "%C0000", "%K%N", "%K%O", "%K%P", "%LC%FS", "%T30", "%T60", "%T10", "%T20", "%T45", "%T01",
    "%V%O", "%V%P", "%K%",
    "\20", "%FE%N", "%FE%O%P", "%FE", "%FS", "%LC", "\yf",
    "\", "%N", "%K", "%P", "%T", "%V", "%O")

$wtf_symbols_mo2 = @(
    "%%K%N", "%LC2020", "%T40", "%CFF3F", "%CFFFF", "%C00FF", "%C000F", "%CF55F",
    "%C4FFF", "%C0000", "%CF9FF", "%T25", "%T99", "%04", "%LF", "%T1",
    "%T20", "%T15", "%T60", "%T10", "%T45", "%CF00F", "%T90", "%FE", "%T30", "%T03",
    "%03", "%FS", "%FE", "\20", "\yf",
    "%K%P", "%K%N", "%K%O", "%LC", "%P", "%N", "%O", "%V", "%K", "%W", "@", "\", "%")

function Verify-Args($cmd, $arg_counts) {
    foreach ($a in $arg_counts) {
        if ($a -eq $cmd.Args.Length) {
            return
        }
    }

    Write-Host "Warning: $($cmd.Cmd) must has $arg_counts args, but got $($cmd.Args.Length)."
}

function Args($a) {
    $ret = ""
    if ($a.Length -ge 1) {
        $ret += $a[0]

        for ($i = 1; $i -lt $a.Length; $i += 1) {
           $ret += ",$($a[$i])"
        }
    }

    return $ret
}

function MO1Var($a) {
    if (($a -eq "FSEL") -or ($a -eq "FDATE") -or ($a -eq "FMONTH")) {
        Write-Host "Vairable can not named FSEL, FDATE or FMONTH."
        Break Script
    }

    switch ($a) {
        "F88" { return "FMONTH" }
        "F89" { return "FDATE" }
        "F91" { return "FSEL" }
        default { return $a }
    }
}

function MO2Var($a) {
    if (($a -eq "FSEL") -or ($a -eq "FDATE") -or ($a -eq "FMONTH")) {
        Write-Host "Vairable can not named FSEL, FDATE or FMONTH."
        Break Script
    }

    switch ($a) {
        "F11" { return "FSEL" }
        default { return $a }
    }
}

function Eval-CharPos($pos) {
    $pure = $pos.Replace("CHR_LEFT", "25").Replace("CHR_RIGHT", "75").Replace("CHR_CENTER", "50")
    if ($pure -eq "") { $pure = "50" }
    $pure = $pure.Replace("_O", "")
    return (Invoke-Expression $pure)
}

function Compile-MO1($target, $cmd, $title_list) {
    switch ($cmd.Cmd) {
        "bg" {
            Verify-Args $cmd @(3, 5)

            if ($cmd.Args[1] -eq "") {
                $cmd.Args[1] = "BG_ALPHA"
            }

            if ($cmd.Args[2] -eq "") {
                $cmd.Args[2] = "300"
            }

            $_ = $target.AppendLine("#bg $($cmd.Args[0]),$($cmd.Args[1]),$($cmd.Args[2])")

            break
        }
        "bgtime" {
            Verify-Args $cmd @(2)
            $_ = $target.AppendLine("#bg $($cmd.Args[0]),BG_ALPHA,1000")
            break
        }
        "csp" {
            Verify-Args $cmd @(2, 3, 4)
            $cid = $cmd.Args[0]
            $filename = $cmd.Args[1]
            $pos = "50"
            if ($cmd.Args.Length -gt 2) {
                $pos = Eval-CharPos $cmd.Args[2]
            }
            $_ = $target.AppendLine("#chara $cid,$filename,$pos,0,300")
            break
        }
        "cspw" {
            Verify-Args $cmd @(6)
            $cid1 = $cmd.Args[0]
            $cid2 = $cmd.Args[1]
            $filename1 = $cmd.Args[2]
            $filename2 = $cmd.Args[3]

            $_ = $target.AppendLine("#chara $cid1,$filename1,25,$cid2,$filename2,75,300")
            break
        }
        "cpos" {
            Verify-Args $cmd @(3)
            $cid = $cmd.Args[0]
            $pos = Eval-CharPos $cmd.Args[1]
            $_ = $target.AppendLine("#chara_pos $cid,$pos,0,5")
            break
        }
        "crs" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#chara_pos $cid,50,0,5")
            break
        }
        "cspwtime" {
            Verify-Args $cmd @(5)
            $cid1 = $cmd.Args[0]
            $cid2 = $cmd.Args[1]
            $filename1 = $cmd.Args[2]
            $filename2 = $cmd.Args[3]

            $_ = $target.AppendLine("#chara $cid1,$filename1,25,$cid2,$filename2,75,300")
            break
        }
        "csptime" {
            Verify-Args $cmd @(2, 3, 4)
            $cid = $cmd.Args[0]
            $file = $cmd.Args[1]
            $pos = "50"
            if ($cmd.Args.Length -gt 2) {
                $pos = Eval-CharPos $cmd.Args[2]
            }
            $_ = $target.AppendLine("#chara $cid,$filename,$pos,0,300")
            break
        }
        "fade_out" {
            Verify-Args $cmd @(2, 3)

            $color = "#000000"

            if ($cmd.Args[1] -eq "FADE_WHITE") {
                $color = "#FFFFFF"
            }

            [double]$time = $cmd.Args[0]
            [double]$time = $time * 1000.0 / 60.0
            [int]$time = $time

            $_ = $target.AppendLine("#fade_out $color,$time")
            break
        }
        "select_text" {
            $_ = $target.AppendLine("#select_text $(Args $cmd.Args)")
            break
        }
        "fade_out_sta" {
            Verify-Args $cmd @(2)

            $color = "#000000"

            if ($cmd.Args[1] -eq "FADE_WHITE") {
                $color = "#FFFFFF"
            }

            [double]$time = $cmd.Args[0]
            [double]$time = $time * 1000.0 / 60.0
            [int]$time = $time

            $_ = $target.AppendLine("#fade_out $color,$time")
            break
        }
        "fade_in" {
            Verify-Args $cmd @(0, 1, 2)

            $time = 300
            if ($cmd.Args.Length -gt 0) {
                $time = $cmd.Args[0]
            }

            $_ = $target.AppendLine("#fade_in $time")
            break
        }
        "fade_in_sta" {
            Verify-Args $cmd @(0, 1, 2)

            $time = 300
            if ($cmd.Args.Length -gt 0) {
                $time = $cmd.Args[0]
            }

            $_ = $target.AppendLine("#fade_in $time")
            break
        }
        "scroll" {
            Verify-Args $cmd @(6)
            $_ = $target.AppendLine("#scroll $(Args $cmd.Args)")
            break
        }
        "label" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#label $($cmd.Args[0])")
            break
        }
        "mst" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#bgm_stop")
            break
        }
        "change" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#change $($cmd.Args[0])")
            break
        }
        "woff" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#se_stop")
            break
        }
        "vo" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#vo $($cmd.Args[0])")
            break
        }
        "bgm" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#bgm $($cmd.Args[0])")
            break
        }
        "bgmonce" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#bgm $($cmd.Args[0]),0")
            break
        }
        "eff" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#se $($cmd.Args[0])")
            break
        }
        "sst" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#se_stop")
            break
        }
        "title" {
            Verify-Args $cmd @(1)
            $title = $title_list[$cmd.Args[0]]
            $_ = $target.AppendLine("#title $title")
            break
        }
        "title_dsp" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#title_dsp")
            break
        }
        "end" {
            Verify-Args $cmd @(0, 1)
            $go = $cmd.Args[0]
            if ($cmd.Args.Length -eq 0) { $go = "oini" }
            $_ = $target.AppendLine("#change $go")
            break
        }
        "set" {
            Verify-Args $cmd @(2)
            $_ = $target.AppendLine("#set $(MO1Var $cmd.Args[0]),$(MO1Var $cmd.Args[1])")
            break
        }
        "add" {
            Verify-Args $cmd @(2)
            $_ = $target.AppendLine("#add $(MO1Var $cmd.Args[0]),$(MO1Var $cmd.Args[1])")
            break
        }
        "if" {
            Verify-Args $cmd @(2)
            $condition = $cmd.Args[0].Replace("<>", "!=")
            if ($condition.Contains("FSEL") -or 
                $condition.Contains("FDATE") -or 
                $condition.Contains("FMONTH")) {
                Write-Host "Error! 'if' condition can not test FSEL, FDATE and FMONTH."
                Break Script
            }

            $condition = $condition.Replace("F91", "FSEL").Replace("F88", "FMONTH").Replace("F89", "FDATE")

            $_ = $target.AppendLine("#if $condition,$($cmd.Args[1])")
            
            break
        }
        "goto" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#goto $($cmd.Args[0])")
            break
        }
        "sel" {
            [int]$count = $cmd.Args[0]
            [int]$count_1 = $count + 1
            [int]$count_2 = $count + 2
            Verify-Args $cmd @($count_1, $count_2)

            if ($cmd.Args[1] -eq "HINT_NONE") {
                $cmd.Args[1] = $cmd.Args[0]
                $cmd.Args = $cmd.Args[1..($cmd.Args.Length - 1)]
            }

            $selections = $null
            if ($count_1 -eq $cmd.Args.Length) {
                $_ = $target.AppendLine("#sel $($cmd.Args[0])")
                $selections = $cmd.Args[1 .. ($cmd.Args.Length - 1)]
            } else {
                $_ = $target.AppendLine("#sel $($cmd.Args[0]),$($cmd.Args[1])")
                $selections = $cmd.Args[2 .. ($cmd.Args.Length - 1)]
            }

            foreach ($selection in $selections) {
                $_ = $target.AppendLine($selection)
            }

            break
        }
        "wait" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#wait $($cmd.Args[0])")
            break
        }
        "EXT_CALL" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#call $($cmd.Args[0])")
            break
        }
        "select_img" {
            [int]$selections = $cmd.Args[0]
            [int]$arg_count = $selections * 3 + 3
            Verify-Args $cmd @($arg_count)

            for ($sel_id = 0; $sel_id -lt $selections; $sel_id += 1) {
                $cmd.Args[2 + $sel_id * 3 + 2] = MO1Var($cmd.Args[2 + $sel_id * 3 + 2])
            }

            $_ = $target.AppendLine("#select_img $(Args $cmd.Args)")
            break
        }
        "load" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#load")
            break
        }
        "movie" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#movie $(Args $cmd.Args)")
            break
        }
        "scr_calen" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#date $($cmd.Args[0]),60,42,#4080B1")
            break
        }
        "ALBUM" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#album")
            break
        }
        "MUSIC" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#music")
            break
        }
        default {
            # Write-Host "Unsupported command $($cmd.Cmd)"
            break
        }
    }
}

function Compile-MO2($target, $cmd) {
    switch ($cmd.Cmd) {
        "ALBUM" { Compile-MO1 $target $cmd $null; break }
        "MUSIC" { Compile-MO1 $target $cmd $null; break }
        "load"  { Compile-MO1 $target $cmd $null; break }
        "label" { Compile-MO1 $target $cmd $null; break }
        "change"{ Compile-MO1 $target $cmd $null; break }
        "goto"{ Compile-MO1 $target $cmd $null; break }
        "select_text"{ Compile-MO1 $target $cmd $null; break }
        "sel"{ Compile-MO1 $target $cmd $null; break }

        "if" {
            Verify-Args $cmd @(2)

            $condition = $cmd.Args[0].Replace("<>", "!=")
            if ($condition.Contains("FSEL") -or 
                $condition.Contains("FDATE") -or 
                $condition.Contains("FMONTH")) {
                Write-Host "Error! 'if' condition can not test FSEL, FDATE and FMONTH."
                Break Script
            }

            if ($condition.Contains("!") -and -not $condition.Contains("!=")) {
                $condition = $condition.Replace("!", "!=")
            }

            $condition = $condition.Replace("F11", "FSEL")

            $_ = $target.AppendLine("#if $condition,$($cmd.Args[1])")
            
            break
        }

        "select_img" {
            [int]$selections = $cmd.Args[0]
            [int]$arg_count = $selections * 3 + 3
            Verify-Args $cmd @($arg_count)

            for ($sel_id = 0; $sel_id -lt $selections; $sel_id += 1) {
                $cmd.Args[2 + $sel_id * 3 + 2] = MO2Var($cmd.Args[2 + $sel_id * 3 + 2])
            }

            $_ = $target.AppendLine("#select_img $(Args $cmd.Args)")
            break
        }

        "BG_DSP" {
            Verify-Args $cmd @(1, 2, 3)

            if ($cmd.Args.Length -eq 1) {
                $_ = $target.AppendLine("#bg $($cmd.Args[0])")
            } else {
                $_ = $target.AppendLine("#bg $($cmd.Args[0]),$($cmd.Args[1]),$($cmd.Args[2])")
            }
            break
        }

        "CG_DSP" {
            Verify-Args $cmd @(4)
            $_ = $target.AppendLine("#bg $($cmd.Args[0])")
            break
        }

        "BG_PART" {
            Verify-Args $cmd @(3)
            $_ = $target.AppendLine("#bg $($cmd.Args[0]),BG_ALPHA,BG_NORMAL,$($cmd.Args[1]),$($cmd.Args[2])")
            break
        }

        "MOV_PLY" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#movie $($cmd.Args[0])")
            break
        }

        "FLUSH" {
            Verify-Args $cmd @(1, 2)
            $col = "#FFFFFF"

            switch ($cmd.Args[0]) {
                "RED" { $col = "#FF0000"; break }
                "WHITE" { $col = "#FFFFFF"; break }
                "BLUE" { $col = "#0000FF"; break }
                default { $col = $cmd.Args[0]; break }
            }

            $time = 300
            if ($cmd.Args.Length -gt 1) {
                $time = $cmd.Args[1]
            }

            $_ = $target.AppendLine("#flash $col,$time")
            break
        }

        "CHR_DSPM" {
            [int]$chrs = $cmd.Args.Length / 3
            if ($chrs -eq 0) {
                Write-Host "CHR_DSPM don't has 0 chara."
                Break Script
            }

            $chr_param = @()
            
            for ($chr_id = 0; $chr_id -lt $chrs; $chr_id += 1) {
                $chr_param += [string]$chr_id
                $chr_param += $cmd.Args[$chr_id * 3]
                $chr_param += Eval-CharPos ($cmd.Args[$chr_id * 3 + 1])
                $chr_param += $cmd.Args[$chr_id * 3 + 2]
            }

            $chr_param += "300"

            $_ = $target.AppendLine("#chara $(Args $chr_param)")

            break
        }

        "CHR_ERSW" {
            Verify-Args $cmd @(0, 1)
            $_ = $target.AppendLine("#chara_cls a")
            break
        }

        "BGM_STA" {
            Verify-Args $cmd @(1, 2, 3)
            $_ = $target.AppendLine("#bgm $($cmd.Args[0])")
            break
        }

        "BGM_STP" {
            Verify-Args $cmd @(0, 1)
            $_ = $target.AppendLine("#bgm_stop")
            break
        }

        "SE_STA" {
            Verify-Args $cmd @(1, 2, 3)
            $_ = $target.AppendLine("#se $($cmd.Args[0])")
            break
        }

        "SE_STP" {
            Verify-Args $cmd @(0, 1)
            $_ = $target.AppendLine("#se_stop")
            break
        }

        "RSET" {
            Verify-Args $cmd @(2)
            $_ = $target.AppendLine("#set $(MO2Var $cmd.Args[0]),$(MO2Var $cmd.Args[1])")
            break
        }

        "add" {
            Verify-Args $cmd @(2)
            $_ = $target.AppendLine("#add $(MO2Var $cmd.Args[0]),$(MO2Var $cmd.Args[1])")
            break
        }

        "sub" {
            Verify-Args $cmd @(2)
            $_ = $target.AppendLine("#sub $(MO2Var $cmd.Args[0]),$(MO2Var $cmd.Args[1])")
            break
        }

        "VO_STA" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#vo $($cmd.Args[0])")
            break
        }

        "VIB_COLLISION_L" {
            Verify-Args $cmd @(0, 1)
            $_ = $target.AppendLine("#quake")
            break
        }        

        "WAIT" {
            Verify-Args $cmd @(1)
            [double]$time = $cmd.Args[0]
            $time = $time / 1000.0 * 70.0
            [int]$time = $time
            $_ = $target.AppendLine("#wait $time")
            break
        }

        "config" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#config")
            break
        }

        #################### ↑ 缘之空截止到这里 ####################
        #################### ↓    秋之回忆2   ####################

        "SCRMODE" { break }
        "FILT_IN" { break }
        "SET_SAVPNT" { break }
        "CLR_SAVPNT" { break }
        "FILT_OUT" { break }
        "ICON_SET" { break }
        "EFF_SUN_HI_L_STA" { break }
        "EFF_SUN_HI_R_STA" { break }
        "EFF_SUN_YU_L_STA" { break }
        "DIS_KEY" { break }
        "OMAKE_OPEN" { break }
        "EFF_SUN_STP" { break }
        "VIB_STP" { break }
        "CHR_COL" { break }
        "VIB_QUAKE_L2_STA" { break }
        "VIB_QUAKE_L3_STA" { break }
        "WIN_OFF" { break }
        "BG_UV_MOVE" { break }
        "VO_WAT" { break }
        "DIS_SKIP" { break }
        "EFF_RAIN_STP" { break }
        "ENA_KEY" { break }
        "ENA_SKIP" { break }
        "RRND" { break }
        "EFF_QUA_STA" { break }
        "EFF_STP" { break }
        "BGM_VOL" { break }
        "SE_WAT" { break }
        "EFF_COLLISION_L" { break }
        "EFF_STA" { break }
        "CHR_PRI" { break }
        "BG_FLG" { break }
        "BG_UV_AUTO" { break }
        "BG_UV_SAVE" { break }
        "LOGO_CHECK" { break }
        "VIB_QUAKE_L1_STA" { break }
        "EFF_SUN_YU_R_STA" { break }
        "CHR_QUA" { break }
        "BG_DSP2" { break }
        "CHR_ERSTC" { break }
        "EFF_RAIN_L2_M_STA" { break }
        "EFF_SUN_M1HI_R1_STA" { break }
        "EFF_SUN_M1HI_L2_STA" { break }
        "FADE_OUT_STA" { break }
        "KEY_WAIT2" { break }
        "QUA" { break }
        "FADE_OUT_WAT" { break }
        "BG_DSPEX" { break }
        "EFF_SNOW_STA" { break }
        "EFF_SNOW_STP" { break }
        "EFF_RAIN_L3_M_STA" { break }
        "EFF_RAIN_L1_M_STA" { break }
        "EFF_PAR" { break }
        "CG_SCRL_BT" { break }
        "VO_STP" { break }
        "EFF_TRAIN" { break }
        "VO_PLY" { break }
        "SE_PLY" { break }

        "scroll" { Compile-MO1 $target $cmd $null; break }

        "GOTO_ENDING" {
            Verify-Args $cmd @(0, 1, 2)
            $scr = "oini"
            if ($cmd.Args.Length -gt 0) {
                $scr = $cmd.Args[0]
            }

            $_ = $target.AppendLine("#change $scr")
            break
        }

        "CHR_DSPW" {
            Verify-Args $cmd @(3, 5, 6)
            
            [int]$cid1 = $cmd.Args[0]
            [int]$cid2 = 1 - $cid1

            $filename1 = $cmd.Args[1]
            $filename2 = $cmd.Args[2].Substring(0, 6)

            if ($filename1.Length -gt 6) {
                $filename1 = $filename1.Substring(0, 6)
            }

            if ($filename2.Length -gt 6) {
                $filename2 = $filename2.Substring(0, 6)
            }

            $pos1 = "25"
            $pos2 = "75"

            if ($cmd.Args.Length -gt 3) {
                if (-not [System.String]::IsNullOrWhitespace($cmd.Args[3])) {
                    $pos1 = Eval-CharPos $cmd.Args[3]
                }
            }

            if ($cmd.Args.Length -gt 4) {
                if (-not [System.String]::IsNullOrWhitespace($cmd.Args[4])) {
                    $pos2 = Eval-CharPos $cmd.Args[4]
                }
            }

            $_ = $target.AppendLine("#chara a,300")
            $_ = $target.AppendLine("#chara $cid1,$filename1,$pos1,0,$cid2,$filename2,$pos2,0,300")
            break
        }

        "CHR_ERS" {
            Verify-Args $cmd @(1)
            $cid = $cmd.Args[0]
            if ($cid -eq "3") {
                $cid = "a"
            }

            $_ = $target.AppendLine("#chara_cls $cid")

            break
        }

        "CHR_POSC" {
            Verify-Args $cmd @(2, 3)
            $cid = $cmd.Args[0]
            $pos = Eval-CharPos $cmd.Args[1]
            $_ = $target.AppendLine("#chara_pos $cid,$pos,0,5")
            break
        }

        "SCR_CALEN" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#date $($cmd.Args[0]),70,42,#4080B1")
            break
        }

        "BGM_STA" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#bgm $($cmd.Args[0])")
            break
        }
        
        "TITLE" {
            Verify-Args $cmd @(1)
            $_ = $target.AppendLine("#title $($cmd.Args[0])")
            break
        }

        "TITLE_DSP" {
            Verify-Args $cmd @(0)
            $_ = $target.AppendLine("#title_dsp")
            break
        }

        "FADE_OUT" {
            Verify-Args $cmd @(1, 2, 3)
            $col = "#000000"

            if ($cmd.Args.Length -gt 1) {
                switch ($cmd.Args[1]) {
                    "FADE_BLACK" { $col = "#000000"; break }
                    "FADE_WHITE" { $col = "#FFFFFF"; break }
                    default { $col = $cmd.Args[1]; break }
                }
            }

            [double]$time = $cmd.Args[0]
            $time = $time * 1000 / 60
            [int]$time = $time

            $_ = $target.AppendLine("#fade_out $col,$time")
            break
        }

        "FADE_IN" {
            Verify-Args $cmd @(0, 1)

            [double]$time = 300.0
            if ($cmd.Args.Length -gt 0) {
                [double]$time = $cmd.Args[0]
            }

            $time = $time * 1000 / 60
            [int]$time = $time

            $_ = $target.AppendLine("#fade_in $time")
            break
        }

        "CHR_DSP" {
            Verify-Args $cmd @(2, 3, 4)
            $pos = "50"

            if ($cmd.Args.Length -gt 2) {
                $pos = Eval-CharPos $cmd.Args[2]
            }

            $filename = $cmd.Args[1]
            if ($filename.Length -gt 6) {
                $filename = $filename.Substring(0, 6)
            }

            $_ = $target.AppendLine("#chara_cls a,300")
            $_ = $target.AppendLine("#chara $($cmd.Args[0]),$filename,$pos,0,300")
            break
        }

        default {
            Write-Host "Unsupported command $($cmd.Cmd)"
        }
    }
}

function Compile-Script($lines, $is_mo_1, $mo1_title_list) {
    $target = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $lines.Length; $i += 1) {
        $line = $lines[$i].Trim()

        $comment_sep = $line.IndexOf(';')
        $comment = ""

        if (-not $is_mo_1) {
            if ($line.StartsWith("#SELECT ")) {
                $line = $line.Replace("#SELECT ", "#sel ")
            }
        }

        if ($line.Contains(";`tVIB_DOKA")) {
            $_ = $target.AppendLine("#quake")
        } elseif ($line.Contains(";`tSE_PLY")) {
            $line = $line.Substring(8)
            $arg = $line.Split(',')[0].Trim()
            if ($arg.Contains("//")) {
                $arg = $arg.Substring(0, $arg.IndexOf("//")).Trim()
            }

            $_ = $target.AppendLine("#se $arg")
        } elseif ($line.Contains(";`tRET") -or $line.Contains(";`t`tRET")) {
            $_ = $target.AppendLine("#ret")
        } elseif ($comment_sep -ge 0) {
            $comment = $line.Substring($comment_sep);
            $line = $line.Substring(0, $comment_sep).Trim();
        } elseif ([System.String]::IsNullOrWhitespace($line)) {
            # Empty line
        } elseif ($line.StartsWith("#")) {
            $cmd_sep = $line.IndexOf(' ')
            $cmd = $line
            $args = @()
            if ($cmd_sep -gt 0) {
                $cmd = $line.Substring(0, $cmd_sep);
                $args = $line.Substring($cmd_sep).Trim().Split(',');
            }

            $cmd = $cmd.Trim().TrimStart('#').Trim()
            for ($j = 0; $j -lt $args.Length; $j += 1) {
                $args[$j] = $args[$j].Trim()
            }

            if ($cmd -eq "") {
                Write-Host "Warning($i): Empty command!"
            }

            if ($cmd -eq "sel") {
                $sel_count = [int]$args[0]
                for ($sel_id = 0; $sel_id -lt $sel_count; $sel_id += 1) {
                    $args += $lines[$sel_id + $i + 1].Trim()
                }

                $i += $sel_id
            }

            $element = @{ 
                Cmd = $cmd
                Args = $args
                Comment = $comment 
            }

            if ($is_mo_1) {
                Compile-MO1 $target $element $title_list
            } else {
                Compile-MO2 $target $element
            }

        } elseif ($line.StartsWith('*')) {
            # discard
        } else {
            # Say command
            $say_lines = @($line)

            :break_say while ($is_mo_1) {
                $i += 1
                $line = $lines[$i]

                if ($line -eq $null) { break break_say }

                $comment_sep = $line.IndexOf(';')
                if ($comment_sep -ge 0) {
                    $line = $line.Substring(0, $comment_sep).Trim()
                } else {
                    $line = $line.Trim()
                }

                if ($line -eq "*") { 
                    break break_say
                } elseif ($line.StartsWith("#")) {
                    $i -= 1
                    break break_say
                }

                $say_lines += $line
            }

            $wtf_symbols = $wtf_symbols_mo2

            if ($is_mo_1) { $wtf_symbols = $wtf_symbols_mo1 }

            for ($say_id = 0; $say_id -lt $say_lines.Length; $say_id += 1) {
                $l = $say_lines[$say_id]

                foreach ($wtf in $wtf_symbols) {
                    $l = $l.Replace($wtf, "")
                }

                if ($l.Contains("@") -or $l.Contains("%") -or $l.Contains("\")) {
                    Write-Host "Warning $i : $l"
                }

                $say_lines[$say_id] = $l.Trim()
            }

            $char_name = ""

            if ($say_lines.Length -gt 0) {
                if ($is_mo_1) {
                    if ($say_lines[0].StartsWith("【") -and $say_lines[0].EndsWith("】")) {
                        $char_name = $say_lines[0].Replace("【", "").Replace("】", "").Trim()
                        $say_lines = $say_lines[1..($say_lines.Length - 1)]
                    }
                } else {
                    if ($say_lines[0].Contains("「") -and $say_lines[0].Contains("」")) {
                        $ch_sep = $say_lines[0].IndexOf('「')
                        if ($ch_sep -gt 0) {
                            $char_name = $say_lines[0].Substring(0, $ch_sep)
                            $say_lines[0] = $say_lines[0].Substring($ch_sep)
                        }
                    }
                }
                if ($say_lines.Length -gt 0) {
                    $_ = $target.Append("#say ")
                    if ($char_name -ne "") {
                        $_ =  $target.Append($char_name).Append(",")
                    }

                    $_ = $target.Append($say_lines[0])

                    for ($say_id = 1; $say_id -lt $say_lines.Length; $say_id += 1) {
                        $_ = $target.Append("\n").Append($say_lines[$say_id])
                    }

                    $_ = $target.AppendLine()
                }
            }
        }
    }

    return ($target.ToString())
}

function Clean-Patch {
    if (Test-Path "gameconfig-bak.txt") {
        if (Test-Path "gameconfig.txt") {
            Remove-Item "gameconfig.txt"
        }

        Rename-Item "gameconfig-bak.txt" "gameconfig.txt"
    }

    if (Test-Path "script-bak") {
        if (Test-Path "script") {
            Remove-Item "script" -Recurse
        }

        Rename-Item "script-bak" "script"
    }
}

function Apply-Patch {
    Clean-Patch
    $is_mo1 = Process-GameConfig

    if (-not (Test-Path "script")) {
        Write-Host "找不到script目录。"
        Break Script
    }

    Rename-Item "script" "script-bak"
    mkdir "script"

    foreach ($i in $non_script) {
        $src = "script-bak/$i"
        $dst = "script/$i"
        if (Test-Path $src) {
            Copy-Item -Path $src -Destination $dst
        }
    }

    if ($is_mo1) {
        $title_list = Parse-GameConfig("script/title_list.txt")

        foreach ($script_file in (Get-ScriptList "script-bak")) {
            $script_filename = $script_file.Name

            $src_file = "script-bak/$script_filename"
            $dst_file = "script/$script_filename"

            Write-Host "Compiling MO1 $script_filename..."

            Compile-Script ([System.IO.File]::ReadAllLines($src_file)) $true $title_list `
            | Out-File -FilePath $dst_file -Encoding utf8
        }
    } else {
        foreach ($script_file in (Get-ScriptList "script-bak")) {
            $script_filename = $script_file.Name

            $src_file = "script-bak/$script_filename"
            $dst_file = "script/$script_filename"

            Write-Host "Compiling $script_filename..."

            Compile-Script ([System.IO.File]::ReadAllLines($src_file)) $false $null `
            | Out-File -FilePath $dst_file -Encoding utf8
        }
    }
}

<#
### Debugging Compiler
foreach ($script_file in (Get-ScriptList "script")) {
    $script_filename = $script_file.Name

    $src_file = "script/$script_filename"

    Write-Host "Compiling $script_filename..."

    $code = Compile-Script ([System.IO.File]::ReadAllLines($src_file)) $false $null
}
#>

Apply-Patch

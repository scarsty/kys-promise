
@echo off
setlocal

cd /d "%~dp0"

set path_project=%~dp0
set path_ct=%path_project%..\codetyphon
set project=kys_promise.lpr
set output=kys_promise.exe

if not exist release mkdir release

rem powershell -NoProfile -Command "$lpi=[xml](Get-Content 'kys_promise.lpi'); $maj=$lpi.SelectSingleNode('//MajorVersionNr').GetAttribute('Value'); $min_=$lpi.SelectSingleNode('//MinorVersionNr').GetAttribute('Value'); $rev=$lpi.SelectSingleNode('//RevisionNr').GetAttribute('Value'); $nv=\"$maj.$min_.$rev.0\"; $b=[IO.File]::ReadAllBytes('kys_promise.res'); $u=[Text.Encoding]::Unicode; $s=$u.GetString($b); $m=[regex]::Match($s,'\d+\.\d+\.\d+\.\d+'); if($m.Success -and $m.Value -ne $nv){ $fi=$u.GetBytes($m.Value); $re=$u.GetBytes($nv); if($fi.Length -eq $re.Length){ for($i=0;$i -lt $b.Length-$fi.Length;$i++){$ok=1;for($j=0;$j -lt $fi.Length;$j++){if($b[$i+$j]-ne$fi[$j]){$ok=0;break}};if($ok){for($j=0;$j -lt $re.Length;$j++){$b[$i+$j]=$re[$j]}}}; [IO.File]::WriteAllBytes('kys_promise.res',$b); Write-Host 'Version updated:' $m.Value '->' $nv } else { Write-Host 'Version length differs, skip patch' } } else { Write-Host 'kys_promise.res already at version' $nv }"

"%path_ct%\fpc\fpc64\bin\x86_64-win64\fpc.exe" -MDelphi -Scghi -CX -O3 -gw2 -godwarfsets -gl -gv -Xg -Xs -XX -WG -l -vewnhibq -Firelease -Fl. -Fulib -Fu%path_ct%\typhon\lcl\units\x86_64-win64\win32 -Fu%path_ct%\typhon\lcl\units\x86_64-win64 -Fu%path_ct%\typhon\components\BaseUtils\lib\x86_64-win64 -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\winunits-base -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\rtl-objpas -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-image -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-base -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\paszlib -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\hash -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\pasjpeg -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-process -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\chm -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-json -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-xml -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-extra -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-res -FUrelease -FE. -o%output% -dLCL -dLCLwin32 %project%

if /I not "%~1"=="nopause" pause
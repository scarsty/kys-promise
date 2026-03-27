@echo off
setlocal

set path_project=%~dp0
set path_ct=%path_project%..\..\codetyphon\
if not exist "%path_ct%\fpc\fpc64\bin\x86_64-win64\fpc.exe" set path_ct=D:\kys-all\codetyphon\

set project=kys_promise.lpr
set output=kys_promise.exe

"%path_ct%\fpc\fpc64\bin\x86_64-win64\fpc.exe" -MDelphi -Scghi -CX -O3 -OoREGVAR -gw2 -godwarfsets -gl -gv -Xg -Xs -XX -WG -l -vewnhibq -Firelease -Fl. -Fu%path_project%lib -Fu%path_ct%\typhon\lcl\units\x86_64-win64\win32 -Fu%path_ct%\typhon\lcl\units\x86_64-win64 -Fu%path_ct%\typhon\components\BaseUtils\lib\x86_64-win64 -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\winunits-base -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\rtl-objpas -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-image -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-base -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\paszlib -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\hash -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\pasjpeg -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-process -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\chm -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-json -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-xml -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-extra -Fu%path_ct%\fpc\fpc64\units\x86_64-win64\fcl-res -FUrelease -FE. -o%output% -dLCL -dLCLwin32 %project%

if /I not "%~1"=="nopause" pause
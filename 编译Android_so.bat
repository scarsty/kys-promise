@echo off
setlocal

cd /d "%~dp0"

set path_project=%~dp0
set path_ct=%path_project%..\codetyphon
if not exist "%path_ct%\fpc\fpc64\bin\x86_64-win64\fpc.exe" set path_ct=D:\kys-all\codetyphon

set project=kys_promise.lpr
set output=libkys_promise.so

if not exist android mkdir android

"%path_ct%\fpc\fpc64\bin\x86_64-win64\fpc.exe" -Tandroid -Paarch64 -MDelphi -Scghi -CX -Os2 -Xs -XX -l -vewnhibq -Fiandroid -Fl.\lib -Fl"%path_ct%\binLibraries\android-5.0.x-api21-aarch64" -Fl"%path_ct%\lib\arm64-v8a" -Fl"%path_ct%\arm64-android" -Fulib -Fu"%path_ct%\typhon\lcl\units\aarch64-android\customdrawn" -Fu"%path_ct%\typhon\lcl\units\aarch64-android" -Fu"%path_ct%\typhon\components\BaseUtils\lib\aarch64-android" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\rtl-objpas" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-image" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-base" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\paszlib" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\hash" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\pasjpeg" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-process" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\chm" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-json" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-xml" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\pthreads" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\rtl-generics" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-extra" -Fu"%path_ct%\fpc\fpc64\units\aarch64-android\fcl-res" -FUandroid -FE. -o%output% -dLCL -dadLCL -dLCLcustomdrawn %project%

if /I not "%~1"=="nopause" pause
@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem 本脚本位于 kys-promise-androidstudio\ 下
rem path_project = 上级目录 (kys-promise\)
set "path_project=%~dp0..\"

rem --- 查找 CodeTyphon ---
set "path_ct="
if defined CODETYPHON_DIR if exist "!CODETYPHON_DIR!\fpc\fpc64\bin\x86_64-win64\fpc.exe" (
    set "path_ct=!CODETYPHON_DIR!"
)
if not defined path_ct if exist "D:\kys-all\codetyphon\fpc\fpc64\bin\x86_64-win64\fpc.exe" (
    set "path_ct=D:\kys-all\codetyphon"
)
if not defined path_ct if exist "C:\codetyphon\fpc\fpc64\bin\x86_64-win64\fpc.exe" (
    set "path_ct=C:\codetyphon"
)
if not defined path_ct if exist "D:\codetyphon\fpc\fpc64\bin\x86_64-win64\fpc.exe" (
    set "path_ct=D:\codetyphon"
)
if not defined path_ct (
    echo [ERROR] CodeTyphon not found. Set CODETYPHON_DIR env var or install to D:\kys-all\codetyphon
    exit /b 1
)
echo [INFO] CodeTyphon: !path_ct!
echo [INFO] Project:    !path_project!

set project=kys_promise.lpr
set output=libkys_promise.so

"!path_ct!\fpc\fpc64\bin\x86_64-win64\fpc.exe" -Tandroid -Paarch64 -MDelphi -Scghi -CX -Os2 -Xs -XX -l -vewnhibq -Fi"!path_project!tmp\android" -Fl"!path_project!lib" -Fl"!path_ct!\binLibraries\android-5.0.x-api21-aarch64" -Fl"!path_ct!lib\arm64-v8a" -Fu"!path_project!lib" -Fu"!path_ct!\typhon\lcl\units\aarch64-android\customdrawn" -Fu"!path_ct!\typhon\lcl\units\aarch64-android" -Fu"!path_ct!\typhon\components\BaseUtils\lib\aarch64-android" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\rtl-objpas" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-image" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-base" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\paszlib" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\hash" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\pasjpeg" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-process" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\chm" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-json" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-xml" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\pthreads" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\rtl-generics" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-extra" -Fu"!path_ct!\fpc\fpc64\units\aarch64-android\fcl-res" -FU"!path_project!android" -FE. -o%output% -dLCL -dadLCL -dLCLcustomdrawn "!path_project!%project%"
if errorlevel 1 ( echo [ERROR] FPC compile failed & exit /b 1 )

copy /y %output% ".\app\lib\arm64-v8a\" >nul

call gradlew.bat assembleRelease
if errorlevel 1 ( echo [ERROR] Gradle build failed & exit /b 1 )

copy /y ".\app\build\outputs\apk\release\*.apk" . >nul

rem pause
@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"


if not exist build mkdir build
pushd build
REM unpack command arguments-----------------------------------
for %%a in (%*) do set "%%~a=1"
if not "%msvc%"=="1" if not %"clang%"=="1" set msvc=1 
if not "%release%"=="1"                    set debug="1"
if "%debug%"=="1"                          set release="0" && echo [debug mode]
if "%release%"=="1"                        set debug="0" && echo [release mode]
if "%msvc%"=="1"                           set clang=0 &&   echo [msvc compile]
if "%clang%"=="1"                          set msvc=0 &&    echo [clang compile]
if "%~1" ==""                              echo [default mode: debug] && set debug=1 
if "%~1" =="relese" if "%~2"==""           echo [release mode] && set release=1 

REM TODO(werkor): figure out how to make the compile arguments work
REM compile arguments-----------------------------------------------


REM unpack link definitions-------------------------------------
if "%debug%"==1 set debug_compile_flags=" -std=c11 -g -O0 -Wall -pthread -DDEBUG" && echo "compile flags: "%compile_flags%""
if "%release%"==1 set release_compile_flags=" -std=c11 -g -O2 -Wall -pthread" && echo "compile flags: "%release_compile_flags%""

REM link arguments-----------------------------------
set link_files=" -lSDL3 -lm"

REM compiler info----------------------------------------
set compiler="clang"
set include_files="-I..\vendor\"
set source_files="..\code\c8_main.c"
set output="-o chip8_tut"

REM compile command-------------------------------------
set compile= call %compiler% %compile_flags% %include_files% %source_files% %output% %link_files%
%compile%

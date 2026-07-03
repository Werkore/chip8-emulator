@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

if not exist build then mkdir build

;; unpack command arguments-----------------------------------
for %%a in (%*) do set "%%~a=1"
if not "%release%" set debug="1"
if "%debug%"=="1" set release=0 && echo [debug mode]
if "%release%"==1 set debug=0 && echo [release mode]

;; unpack link definitions-------------------------------------
if "%debug%"==1 set debug_compile_flags=" -std=c11 -g -O0 -Wall -pthread -DDEBUG" && echo "compile flags: "%compile_flags%"" 
if "%release%"==1 set release_compile_flags=" -std=c11 -g -O2 -Wall -pthread" && echo "compile flags: "%release_compile_flags%""

;; link arguments-----------------------------------
set link_files=" -lSDL3 -lm"

;; compiler info----------------------------------------
set compiler="clang"
set include_files="-I..\vendor\"
set source_files="..\code\c8_main.c"
set output="-o chip8_tut"

;; compile command-------------------------------------
set compile= call %compiler% %compile_flags% %include_files% %source_files% %output% %link_files%
%compile%

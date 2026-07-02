#!/usr/bin/env bash
set -eu

cd "$(dirname "$0")"
for arg in "$@"; do declare $arg='1'; done

if [ ! -d "build" ]; then
  mkdir -p build;
fi
pushd build > /dev/null

# command line arguments-------------------------------------------
# todo(werkor): args arent being set correctly(release is not being set when on command line
# also it automatically picks debug for some reason)
if [ ! -v release ] && [ -v debug ];   then debug=1;   echo "[debug mode enabled]"; fi
if [ ! -v debug ] && [ -v release ];   then release=1; echo "[release mode enabled]"; fi
if [ $# -eq 0 ];                       then release=1; echo "[no arguments,default mode: release enabled]"; fi
if [ ! -v release ] && [ ! -v debug ]; then echo "please enter either debug or release as an argument"; exit 1; fi

# unpack command line arguments-----------------------------------------------
# remove -fsanitize=address because it annoys me
# -Werror
compile_flags="";
if [ -v debug ];    then compile_flags="${compile_flags} -std=c11 -g -O0 -Wall -pthread -D_GNU_SOURCE -DDEBUG"; echo "compile_flags:"${compile_flags}""; fi
if [ -v release ];  then compile_flags="${compile_flags} -std=c11 -O2 -Wall -Werror -pthread -D_GNU_SOURCE"; echo "compile flags: "${compile_flags}""; fi

# link files------------------------------------------------
link_files="";
link_files="${link_files} -lSDL3 -lm";
# link_files="${link_files} -L../vendor/shared_object -Wl,-rpath,"\$ORIGIN/../vendor/shared_object" -l:libSDL3.so.0.4.8 -lm";

# compile info-----------------------------------------------------
compiler="clang";
# include_files= "";
include_files="-I../vendor/"
source_files="../code/c8_main.c";
output="-o chip8_tut";

# compile command---------------------------------------------
compile="$compiler "${compile_flags}" "${include_files}" "${source_files}" "${output}" "${link_files}"";
echo "compile command: "${compile}"";
$compile;



# old comiler command-----------------------------
# include='-I ../code/'
# source_files='../code/c8_main.c'
# header_files='../code/header c8_memory.h'
# link_flags='-lSDL3'
# optional includes: -fsanitize=address -O2
# args='-std=c11 -g -O2 -Wall -D_GNU_SOURCE'
# compiler='clang'

# clang $args $source_files -o c8 $link_flags

popd > /dev/null

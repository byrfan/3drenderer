#!/usr/bin/env bash

CC="gcc"
flags="-lraylib"
src="./src"
bld="./build"
exe="app"
mkdir -p "$bld/src/"

echo "Compiling..."
for file in "$src"/*.c; do
  $CC $flags -c "$file" -o "$bld/src/$(basename "$file" .c).o"
done

echo "Linking..."
$CC "$bld/src"/*.o -o "$bld/$exe" $flags

echo "Do: 
$bld/$exe"

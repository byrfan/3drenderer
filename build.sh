#!/usr/bin/env bash

CC="gcc"
sdl_flags=$(pkg-config --cflags --libs sdl2)
flags="-lm -lSDL2_ttf"
src="./src"
bld="./build"
exe="app"
mkdir -p "$bld/src/"

echo "Compiling..."
for file in "$src"/*.c; do
  $CC $flags -c "$file" -g -o "$bld/src/$(basename "$file" .c).o"
done

echo "Linking..."
$CC "$bld/src"/*.o -o "$bld/$exe" $sdl_flags $flags

echo "Do: 
$bld/$exe"

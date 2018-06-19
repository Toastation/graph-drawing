CC=g++
CFLAGS=-std=c++11 -Wno-deprecated-declarations -DNDEBUG
lib_dir=../dependencies/Tulip/lib/
include_dir_lib=../dependencies/Tulip/include/
include_dir=../include/
build_dir=../bin/

all: main

main: main.cpp fmmm_incremental.cpp
		$(CC) $(CFLAGS) $^ $(lib_dir)libtulip-core-5.1.dll $(lib_dir)libtulip-ogl-5.1.dll $(lib_dir)libftgl-tulip-5.1.dll -I$(include_dir_lib) -I$(include_dir) -o $(build_dir)$@
CC=g++
CFLAGS=-std=c++11 -pedantic -Wno-deprecated-declarations -DNDEBUG
lib_dir="C:/Program Files/Tulip-5.1.0/bin/"
include_dir_tulip="C:/Program Files/Tulip-5.1.0/include/"
include_dir_ogdf=C:/Users/Melvin.Melvin-PC/Desktop/work/ogdf/include/
include_dir=../include/
build_dir=../bin/

all: main

main: main.cpp fmmm_incremental.cpp fmmm_layout.cpp 
		$(CC) $(CFLAGS) $^ $(lib_dir)libtulip-core-5.1.dll $(lib_dir)libtulip-ogl-5.1.dll $(lib_dir)libftgl-tulip-5.1.dll $(lib_dir)libtulip-ogdf-5.1.dll $(lib_dir)libOGDF-tulip-5.1.dll -I$(include_dir) -I$(include_dir_tulip) -I$(include_dir_ogdf) -o $(build_dir)$@
cmake_minimum_required (VERSION 3.1)

project(zzt-puzzle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(PROG_NAME "zzt-puzzle")

set(CMAKE_CXX_FLAGS "-O3 -mtune=native -march=native")

add_executable(${PROG_NAME}
	board.cc
	coord.cc
	generator.cc
	puzzle.cc)
cmake_minimum_required (VERSION 3.1)

project(zzt-puzzle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(PROG_NAME "zzt-puzzle")

# There isn't actually any pragma onces outside headers, but my LLVM IDE
# integration flags header pragmas as non-header pragmas due to a known bug.
set(CMAKE_CXX_FLAGS
	"-O3 -mtune=native -march=native -Wall -Werror -Wno-pragma-outside-header")

add_executable(${PROG_NAME}
	board.cc
	coord.cc
	generator.cc
	puzzle.cc)
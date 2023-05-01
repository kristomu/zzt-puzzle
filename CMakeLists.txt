cmake_minimum_required (VERSION 3.1)

project(zzt-puzzle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(PROG_NAME "zzt-puzzle")
set(WRITER_PROG_NAME "writer")

# There isn't actually any pragma onces outside headers, but my LLVM IDE
# integration flags header pragmas as non-header pragmas due to a known bug.
# Werror and Wextra is currently disabled because linux-reconstruction triggers
# too many of them.
set(CMAKE_CXX_FLAGS
	"-O3 -mtune=native -march=native -Wall \
	 -Wno-error=unknown-pragmas \
	 -Wno-pragma-once-outside-header -fopenmp")

add_executable(${PROG_NAME}
	board.cc
	coord.cc
	generator.cc
	puzzle.cc
	solver/dfs.cc
	random/random.cc)

add_executable(${WRITER_PROG_NAME}
	board.cc
	coord.cc
	generator.cc
	solver/dfs.cc
	random/random.cc
	linux-reconstruction-of-zzt/csrc/world.cc
	linux-reconstruction-of-zzt/csrc/board.cc
	linux-reconstruction-of-zzt/csrc/tools.cc
	linux-reconstruction-of-zzt/csrc/serialization.cc
	linux-reconstruction-of-zzt/csrc/elements/info.cc
	writer.cc)

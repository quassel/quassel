# This Makefile simply runs cmake from the build directory.
# This, of course, triggers an out-of-source build.
# Binaries are going to be in build/.

default_target: run_cmake

run_cmake:
	cd build && cmake .. && make

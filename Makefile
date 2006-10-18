# This Makefile simply runs cmake from the build directory.
# This, of course, triggers an out-of-source build.
# Binaries are going to be in build/.

default_target: default

default:
	@echo "To start an out-of-source build, change to the build/ directory"
	@echo "and run 'cmake ..' to create standard Makefiles. You can then use"
	@echo "'make' from the build/ directory to compile the project."
	@echo
	@echo "To create KDevelop3 project files instead, run 'cmake .. -GKDevelop3'"
	@echo "from the build/ directory."
	@echo
	@echo "Please refer to the INSTALL file for more options, such as"
	@echo "client/server builds."
	@echo

run_cmake:
	cd build && cmake .. && make

build_windows:
	cd build && cmake .. -G "MinGW Makefiles" && mingw32-make

clean:
	rm -rf build/*

CMake supports and encourages out-of-source builds, which do not clutter the source directory.
You can (and should) thus use an arbitrary directory for building.
There is no "make distclean"; "make clean" should usually be enough since CMake actually
cleans up properly (qmake often didn't). If you really want to get rid of all build files,
just remove the build directory.

Usually, you should build Quassel as follows:

cd /path/to/build/dir
cmake /path/to/quassel
make

Additionally, you may add some options to the cmake call, prefixed by -D. These need to follow
the source directory PATH:

cmake /path/to/quassel -D<option1> -D<option2>

NOTE: In order to reconfigure, you need to remove CMakeCache.txt (or empty
      the build directory), otherwise cmake will ignore modified -D options!

Quassel recognizes the following options:

-DBUILD=<string>
    Specify which Quassel binaries to build. <string> may contain any combination of
    "core", "client", "mono" or "all".

-DQT=/path/to/qt
    Use a non-system Qt installation. This is for example useful if you have a static
    Qt installed in some local dir.

-DSTATIC=1
    Enable static building of Quassel. You should link the static versions of some libs
    (in particular libstdc++.a) into /path/to/build/dir/staticlibs in oder to create
    a portable binary!

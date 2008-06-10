CMake supports and encourages out-of-source builds, which do not clutter the source directory.
You can (and should) thus use an arbitrary directory for building.
There is no "make distclean"; "make clean" should usually be enough since CMake actually
cleans up properly (qmake often didn't). If you really want to get rid of all build files,
just remove the build directory.

Usually, you will build Quassel as follows:

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

-DSTATICWIN=1
    Enable static building for Windows platforms. This adds some libs that are not automatically
    included for some reason.

BUILDING ON WINDOWS:
--------------------
We have tested building on Windows with a statically built Qt (with its /bin directory in %PATH%)
and MSVC 2005/2008. Make sure that you use a "shell" that has all needed env variables setup,
such as the "Visual Studio Command Prompt". You will also need the /bin of the Microsoft SDK in
your %PATH% at least for VS 2008, otherwise rc.exe is not found.
Currently, only building in the shell using nmake seems to work; CMake can also create MSVC project
files, but they seem to be problematic. However, YMMV.

After you have everything setup:

cd C:\path\to\quassel-build
cmake -G"NMake Makefiles" C:\path\to\quassel\source -DSTATICWIN=1
nmake

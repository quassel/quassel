# This is the cmake-based build system for Quassel IRC.
#
# You may pass various options to cmake:
# -DWANT_(CORE|QTCLIENT|MONO)=(ON|OFF)
#                        : select binaries to build
# -DWITH_OPENSSL=OFF     : Disable OpenSSL support
# -DWITH_DBUS=OFF        : Disable D-Bus support (dbus notifications)
# -DWITH_WEBKIT=OFF      : Disable WebKit support (link previews)
# -DWITH_PHONON=OFF      : Disable Phonon support (audio notifications)
# -DWITH_LIBINDICATE=OFF : Disable libindicate support (Ayatana notifications)
# -DWITH_KDE=ON          : Enable KDE4 support
# -DWITH_OXYGEN=(ON|OFF) : Whether to install Oxygen icons (default: yes, unless KDE > 4.3.0 is present and enabled)
#
# -DEMBED_DATA=ON        : Embed all data files in icons the binary, rather than installing them separately
#
# -DQT=/path/to/qt       : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=ON            : Enable static building of Quassel. Use with care.
# -DDEPLOY=ON            : Mac OS X only. Use only for creating Quassel Packages!
#
# NOTE: You should remove CMakeCache.txt if you plan to change any of these values!

project(QuasselIRC)

# cmake 2.6.2 is required for KDE >=4.2 and should be widespread enough now
cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)

if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Use our own (well, and KDE's) version of some modules
# In particular cmake's own FindQt4 and FindOpenSSL are quite buggy
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
include(QuasselMacros)

# Various options and variables that can be set on the command line
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the Qt4 GUI client binary"          ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" ON)

option(WITH_OPENSSL  "Enable OpenSSL support if present on the system"	ON)
option(WITH_DBUS     "Enable D-Bus support if present on the system"	ON)
option(WITH_WEBKIT   "Enable WebKit support (for link previews)"	ON)
option(WITH_PHONON   "Enable Phonon support (for audio notifications)"	ON)
option(WITH_LIBINDICATE "Enable Ayatana notification support"           ON)
option(WITH_KDE      "Enable KDE4 integration"				OFF)

# We use icon paths from KDE 4.3.x, which are partially invalid on older and possibly
# even on newer KDE versions. Do not disable this unless you are sure that your Quassel will
# run on a matching KDE version only.
set(WITH_OXYGEN AUTO CACHE STRING "Install Oxygen icons (default is \"AUTO\" to install when KDE 4.3 or later is present")

option(STATIC        "Enable static building (might not be portable)" OFF)

if(APPLE)
  option(DEPLOY        "Mac OS X only! Adds required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF)
endif(APPLE)

# Default to embedding data in the static case
if(STATIC OR WIN32)
  set(EMBED_DEFAULT ON)
else(STATIC OR WIN32)
  set(EMBED_DEFAULT ON) # should be OFF as soon as everything works
endif(STATIC OR WIN32)

option(EMBED_DATA    "Embed all data files in the binary (rather than installing them separately)"   ${EMBED_DEFAULT})

set(QT "" CACHE STRING "Path to a Qt installation to use instead of the system Qt (e.g. for static builds)")

# Some settings imply others
if(STATIC)
  add_definitions(-DSTATIC)
  set(WITH_KDE OFF CACHE BOOL "Static building with KDE is not supported")
endif(STATIC)

if(WIN32)
  # We don't support separately installed resources yet on Win32
  set(EMBED_DATA ON)
endif(WIN32)

# For static builds, arbitrary extra libs might need to be linked
# Define a comma-separated list here
# e.g. for pgsql, we need -DLINK_EXTRA=pq;crypt
set(LINK_EXTRA "" CACHE STRING "Semicolon-separated list of libraries to be linked")
if(LINK_EXTRA)
  string(REPLACE "," ";" LINK_EXTRA ${LINK_EXTRA})
  link_libraries(${LINK_EXTRA})
endif(LINK_EXTRA)

# Build Type
# We need to make sure it's not empty
# Supported: Release, RelWithDebugInfo, Debug, Debugfull

# On WIN32, only Release seems to work correctly (?)
if(WIN32)
  set(DEFAULT_BUILD_TYPE "Release")
else(WIN32)
  set(DEFAULT_BUILD_TYPE "RelWithDebugInfo")
endif(WIN32)

set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "CMake Build Type")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "CMake Build Type" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Enable various flags on gcc
if(CMAKE_COMPILER_IS_GNUCXX)
  # Let's just hope that all gccs support these options and skip the tests...
  # -fno-strict-aliasing is needed apparently for Qt < 4.6
  set(CMAKE_CXX_FLAGS                  "${CMAKE_CXX_FLAGS} -ansi -Wall -Wextra -Wnon-virtual-dtor -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS_RELEASE          "-O2 ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBUGINFO "-g -O2 ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG            "-g -ggdb -fno-reorder-blocks -fno-schedule-insns -fno-inline ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUGFULL        "-g3 ${CMAKE_CXX_FLAGS_DEBUG}")
endif(CMAKE_COMPILER_IS_GNUCXX)

string(TOUPPER ${CMAKE_BUILD_TYPE} upper_build_type)
if(upper_build_type STREQUAL "RELEASE" OR upper_build_type STREQUAL "RELWITHDEBUGINFO")
  add_definitions(-DNDEBUG -DQT_NO_DEBUG)
else(upper_build_type STREQUAL "RELEASE" OR upper_build_type STREQUAL "RELWITHDEBUGINFO")
  set(DEBUG 1)
endif(upper_build_type STREQUAL "RELEASE" OR upper_build_type STREQUAL "RELWITHDEBUGINFO")

if(WANT_MONO OR WANT_QTCLIENT)
  set(QT_MIN_VERSION "4.5.0")
else(WANT_MONO OR WANT_QTCLIENT)
  set(QT_MIN_VERSION "4.4.0")
endif(WANT_MONO OR WANT_QTCLIENT)

if(APPLE AND DEPLOY)
  set(CMAKE_OSX_ARCHITECTURES "i386;ppc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4")
  set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
  add_definitions(-DMAC_10_4_SDK)
endif(APPLE AND DEPLOY)

# Execinfo is needed for generating backtraces
find_package(ExecInfo)
if(EXECINFO_FOUND)
  add_definitions(-DHAVE_EXECINFO)
  include_directories(${EXECINFO_INCLUDES})
  link_libraries(${EXECINFO_LIBRARIES})
endif(EXECINFO_FOUND)

# Select a Qt installation here, if you don't want to use system Qt
if(QT)
  # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
  set(ENV{PATH} ${QT}/bin:$ENV{PATH})
endif(QT)

# Now that we have the correct $PATH, lets find Qt!
find_package(Qt4 REQUIRED)

set(QT_DONT_USE_QTGUI 1)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDES})

# Setup OpenSSL
if(WITH_OPENSSL)
  find_package(OpenSSL)
else(WITH_OPENSSL)
  message(STATUS "Not enabling OpenSSL support")
endif(WITH_OPENSSL)

if(OPENSSL_FOUND)
  if(NOT QT_DEFINITIONS MATCHES "QT_NO_OPENSSL")
    message(STATUS "Found OpenSSL support in Qt")
    add_definitions(-DHAVE_SSL)
    set(HAVE_SSL true)
    set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_SSL)
  else(NOT QT_DEFINITIONS MATCHES "QT_NO_OPENSSL")
    message(STATUS "No OpenSSL support found in Qt, disabling")
  endif(NOT QT_DEFINITIONS MATCHES "QT_NO_OPENSSL")
else(OPENSSL_FOUND)
  add_definitions(-DQT_NO_OPENSSL)
endif(OPENSSL_FOUND)

# Setup D-Bus support
if(WITH_DBUS)
  if(QT_QTDBUS_FOUND)
    message(STATUS "Found QtDBus, enabling D-Bus support")
    add_definitions(-DHAVE_DBUS)
    set(LINK_DBUS DBUS)
    set(HAVE_DBUS true)
    set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_DBUS)
  else(QT_QTDBUS_FOUND)
    message(STATUS "QtDBus not found, disabling D-Bus support")
  endif(QT_QTDBUS_FOUND)
else(WITH_DBUS)
  message(STATUS "Not enabling D-Bus support")
endif(WITH_DBUS)

# Setup QtWebKit support
if(WITH_WEBKIT)
  if(QT_QTWEBKIT_FOUND)
    message(STATUS "Found QtWebKit, enabling WebKit support")
    add_definitions(-DHAVE_WEBKIT)
    set(LINK_WEBKIT WEBKIT)
    set(HAVE_WEBKIT true)
    set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_WEBKIT)
  else(QT_QTWEBKIT_FOUND)
    message(STATUS "QtWebKit not found, disabling WebKit support")
  endif(QT_QTWEBKIT_FOUND)
else(WITH_WEBKIT)
  message(STATUS "Not enabling WebKit support")
endif(WITH_WEBKIT)

# Setup KDE4 support
if(WITH_KDE)
  find_package(KDE4)
  if(KDE4_FOUND)
    message(STATUS "Enabling KDE4 integration")
    include_directories(${KDE4_INCLUDES})
    add_definitions(-DHAVE_KDE ${KDE4_DEFINITIONS})
    set(HAVE_KDE 1)
    set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_KDE)
    set(QUASSEL_KDE_LIBRARIES ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBRARY} ${KDE4_SOLID_LIBS} knotifyconfig)
    # We always use external icons for KDE4 support, since we use its iconloader rather than our own
    set(EMBED_DATA OFF)
  else(KDE4_FOUND)
    message(STATUS "KDE4 not found, disabling KDE integration")
  endif(KDE4_FOUND)
else(WITH_KDE)
  message(STATUS "Not enabling KDE4 integration")
endif(WITH_KDE)

# Setup Phonon support - we only need this if we don't have or want KDE4
if(NOT HAVE_KDE)
  if(WITH_PHONON)
    find_package(Phonon)
    if(PHONON_FOUND)
      message(STATUS "Enabling Phonon support")
      add_definitions(-DHAVE_PHONON)
      set(HAVE_PHONON true)
      set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_PHONON)
    else(PHONON_FOUND)
      message(STATUS "Phonon not found, disabling audio notifications")
    endif(PHONON_FOUND)
  else(WITH_PHONON)
    message(STATUS "Not enabling Phonon support")
  endif(WITH_PHONON)
endif(NOT HAVE_KDE)

# Setup libindicate-qt support
if(WITH_LIBINDICATE)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(INDICATEQT indicate-qt>=0.2.1)
    if(INDICATEQT_FOUND)
      message(STATUS "Enabling Ayatana notification support")
      add_definitions(-DHAVE_INDICATEQT)
    else(INDICATEQT_FOUND)
      message(STATUS "Disabling Ayatana notification support")
    endif(INDICATEQT_FOUND)
  endif(PKG_CONFIG_FOUND)
else(WITH_LIBINDICATE)
  message(STATUS "Not enabling Ayatana notification support")
endif(WITH_LIBINDICATE)

# Now set up install locations; those are set by KDE if integration is enabled
if(NOT HAVE_KDE)
  if(WIN32)
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "Install path for binaries")
    set(DATA_INSTALL_DIR $ENV{APPDATA}/quassel-irc.org/share/apps CACHE FILEPATH "Install path for data files")
    set(ICON_INSTALL_DIR $ENV{APPDATA}/quassel-irc.org/share/icons CACHE FILEPATH "Global icon install path")
    set(XDG_APPS_INSTALL_DIR $ENV{APPDATA}/quassel-irc.org/share/applications CACHE FILEPATH "Install path for .desktop files")
  else(WIN32)
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin CACHE FILEPATH "Install path for binaries")
    set(DATA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/apps CACHE FILEPATH "Install path for data files")
    set(ICON_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/icons CACHE FILEPATH "Global icon install path")
    set(XDG_APPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/applications CACHE FILEPATH "Install path for .desktop files")
  endif(WIN32)
endif(NOT HAVE_KDE)

if(EMBED_DATA)
  message(STATUS "Embedding data files into the binary")
else(EMBED_DATA)
  message(STATUS "Installing data files separately")
endif(EMBED_DATA)

# RPATH needs to be set correctly
# Do this down here, since otherwise KDE wants to handle it itself, and fails
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)
set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# Set global buildflags
# This is very much non-portable, so don't use -DSTATIC until you know what
# you do.
if(STATIC AND CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-static-libgcc ${CMAKE_CXX_FLAGS}")
  link_directories(${CMAKE_BINARY_DIR}/staticlibs) # override dynamic libs
  if(HAVE_SSL)
    set(QUASSEL_SSL_LIBRARIES ssl crypto)  # these miss in static builds
  endif(HAVE_SSL)
endif(STATIC AND CMAKE_COMPILER_IS_GNUCXX)

if(WIN32)
  link_libraries(imm32 winmm dbghelp Secur32)  # missing by default :/

  if(HAVE_SSL)
     link_libraries(${OPENSSL_LIBRARIES} libeay32MD)
  endif(HAVE_SSL)
endif(WIN32)

if(INDICATEQT_FOUND)
  add_definitions(-DXDG_APPS_INSTALL_DIR=${XDG_APPS_INSTALL_DIR})
endif(INDICATEQT_FOUND)

# We need to create a version.gen
# For this, we create our genversion binary and make sure it is run every time.
add_executable(genversion ${CMAKE_SOURCE_DIR}/src/common/genversion.cpp)
target_link_libraries(genversion ${QT_LIBRARIES} ${QT_CORE_LIB_DEPENDENCIES})

get_target_property(GENVERSION_EXECUTABLE genversion LOCATION)
add_custom_target(genversion_run ALL ${GENVERSION_EXECUTABLE}
                  ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/src/version.gen)
add_dependencies(genversion_run genversion)

# These variables will be added to the main targets (CORE, QTCLIENT, MONO)
set(COMMON_DEPS ${RC_WIN32})
set(CORE_DEPS )
set(CLIENT_DEPS )

# Add needed subdirs - the order is important, since src needs some vars set by other dirs
add_subdirectory(data)
add_subdirectory(icons)
add_subdirectory(pics)
add_subdirectory(po)
add_subdirectory(src)

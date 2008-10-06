# This is the cmake-based build system for Quassel IRC.
# You may pass various options to cmake:
# -DWANT_(CORE|QTCLIENT|MONO)=(ON|OFF)
#                     : select binaries to build
# -DWITH_OPENSSL=OFF  : Disable OpenSSL support
# -DWITH_DBUS=OFF     : Disable D-Bus support
# -DWITH_WEBKIT=OFF   : Disable WebKit support
# -DOXYGEN_ICONS=(Builtin|External)  : If "Builtin" (the default), compile our Oxygen Icon Theme subset into the binary
#                                    : If "External", we assume Oxygen is already installed on the system
# -DQUASSEL_ICONS=(Builtin|External) : If "Builtin" (the default), put our own icons into the binary
#                                    : If "External", we install our icons into $PREFIX/share/apps/quassel (UNIX only)
# -DQT=/path/to/qt    : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=ON         : Enable static building of Quassel. Use with care.
# -DDEPLOY=ON         : Mac OS X only. Use only for creating Quassel Packages!
#
# NOTE: You need to remove CMakeCache.txt if you plan to change any of these values!

project(QuasselIRC)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Use our own (well, and KDE's) version of some modules
# In particular cmake's FindQt4 and FindOpenSSL are quite buggy
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Various options and variables that can be set on the command line
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the Qt4 GUI client binary"          ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" OFF)

option(WITH_OPENSSL  "Enable OpenSSL support if present on the system"	ON)
option(WITH_DBUS     "Enable D-Bus support if present on the system"	ON)
option(WITH_WEBKIT   "Enable WebKit support if present on the system"	ON)

option(STATIC        "Enable static building (might not be portable)" OFF)
option(DEPLOY        "Mac OS X only! Adds required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF)
option(SPUTDEV       "Do not use!" OFF)

set(OXYGEN_ICONS "Builtin" CACHE STRING "Builtin: Compile Oxygen icons into the binary; External: Use system-installed Oxygen")
set(QUASSEL_ICONS "Builtin" CACHE STRING "Builtin: Compile Quassel icons into the binary; External: Install them separately")

set(QT "" CACHE STRING "Path to a Qt installation to use instead of the system Qt")
set(LINGUAS "" CACHE STRING "Space-separated List of locales specifying languages that should be compiled")

if(STATIC)
  set(CMAKE_BUILD_TYPE Release)
  set(OXYGEN_ICONS "Builtin")
  set(QUASSEL_ICONS "Builtin")
endif(STATIC)

# RPATH needs to be set correctly
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1) 
set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# Define install locations. Using variables will allow overriding this by the KDE macros later.
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set(DATA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/apps)
set(ICON_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/icons)
set(XDG_APPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/applications)

# Enable various flags on gcc
if(CMAKE_COMPILER_IS_GNUCXX)
  # Let's just hope that all gccs support these options and skip the tests...
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi -Wall -Wextra -Wnon-virtual-dtor")
endif(CMAKE_COMPILER_IS_GNUCXX)

set(QT_MIN_VERSION "4.4.0")

if(APPLE AND DEPLOY)
  set(CMAKE_OSX_ARCHITECTURES "i386;ppc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4")
  set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
  add_definitions(-DMAC_10_4_SDK)
endif(APPLE AND DEPLOY)

include(QuasselMacros)

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
  message(STATUS "Disabling OpenSSL support")
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
  message(STATUS "Disabling D-Bus support")
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
    message(STATUS "QtWebKit not found, disabling D-Bus support")
  endif(QT_QTWEBKIT_FOUND)
else(WITH_WEBKIT)
  message(STATUS "Disabling WebKit support")
endif(WITH_WEBKIT)


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

if(STATIC AND WIN32)
  link_libraries(imm32 winmm)  # missing by default :/
   if(HAVE_SSL)
     link_libraries(${OPENSSL_LIBRARIES} libeay32MD)
   endif(HAVE_SSL)
endif(STATIC AND WIN32)

if(WIN32)
  set(RC_WIN32 ../pics/win32.rc)  # for app icons on windows
endif(WIN32)

# This is dirty, but I haven't found a cleaner way to ensure that the generated .qrc files
# (which will be removed with make clean) are regenerated :/
set_directory_properties(PROPERTIES
                         ADDITIONAL_MAKE_CLEAN_FILES CMakeCache.txt)

# We need to create a version.gen
# For this, we create our genversion binary and make sure it is run every time.
add_executable(genversion ${CMAKE_SOURCE_DIR}/src/common/genversion.cpp)
target_link_libraries(genversion ${QT_LIBRARIES} ${QT_CORE_LIB_DEPENDENCIES})

get_target_property(GENVERSION_EXECUTABLE genversion LOCATION)
add_custom_target(genversion_run ALL ${GENVERSION_EXECUTABLE}
                  ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/src/version.gen)
add_dependencies(genversion_run genversion)

# Decide what to do with icons
if(WANT_QTCLIENT OR WANT_MONO)
  if(QUASSEL_ICONS MATCHES "External")
    message(STATUS "Install Quassel icons to ${CMAKE_INSTALL_PREFIX}/share/apps/quassel")
  else(QUASSEL_ICONS MATCHES "External")
    set(QUASSEL_ICONS "Builtin")
    message(STATUS "Compile Quassel icons into the binary")
  endif(QUASSEL_ICONS MATCHES "External")

  if(OXYGEN_ICONS MATCHES "External")
    message(STATUS "Use system-installed Oxygen icon theme")
  else(OXYGEN_ICONS MATCHES "External")
    set(OXYGEN_ICONS "Builtin")
    message(STATUS "Compile Oxygen icon theme subset into the binary")
  endif(OXYGEN_ICONS MATCHES "External")
endif(WANT_QTCLIENT OR WANT_MONO)

# These variables will be added to the main targets (CORE, QTCLIENT, MONO)

set(COMMON_DEPS ${RC_WIN32})
set(CORE_DEPS )
set(CLIENT_DEPS )
set(KDE_DEPS )

# Add needed subdirs - the order is important, since src needs some vars set by other dirs
add_subdirectory(data)
add_subdirectory(icons)
#add_subdirectory(pics)
add_subdirectory(i18n)
add_subdirectory(src)

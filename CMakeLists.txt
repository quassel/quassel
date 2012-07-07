# This is the cmake-based build system for Quassel IRC.
#
# You may pass various options to cmake:
#
# -DWANT_(CORE|QTCLIENT|MONO)=(ON|OFF)
#                        : select binaries to build
# -DWITH_OPENSSL=OFF     : Disable OpenSSL support
# -DWITH_DBUS=OFF        : Disable D-Bus support (dbus notifications)
# -DWITH_WEBKIT=OFF      : Disable WebKit support (link previews)
# -DWITH_PHONON=OFF      : Disable Phonon support (audio notifications)
# -DWITH_LIBINDICATE=OFF : Disable libindicate support (Ayatana notifications)
# -DWITH_KDE=ON          : Enable KDE4 support
# -DWITH_CRYPT=OFF       : Disable encryption support
# -DWITH_OXYGEN=(ON|OFF) : Whether to install Oxygen icons (default: yes, unless KDE > 4.3.0 is present and enabled)
# -DWITH_SYSLOG=OFF      : Disable syslog support
#
# -DEMBED_DATA=ON        : Embed all data files in icons the binary, rather than installing them separately
#
# -DQT=/path/to/qt       : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=ON            : Enable static building of Quassel. Use with care.
# -DDEPLOY=ON            : Mac OS X only. Use only for creating Quassel Packages!
#
# -DWITH_QT5=ON          : Enable (very) experimental support for Qt5 (see doc/README.Qt5!)
#
# NOTE: You should remove CMakeCache.txt if you plan to change any of these values!


project(QuasselIRC)

include(CheckFunctionExists)
include(CheckIncludeFile)

# cmake 2.6.2 is required for KDE >=4.2 and should be widespread enough now
# For building against Qt5, we check for an even newer cmake version below!
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
option(WITH_CRYPT    "Enable encryption support if present on system"	ON)
option(WITH_SYSLOG   "Use syslog for storing log data"			ON)

# We use icon paths from KDE 4.3.x, which are partially invalid on older and possibly
# even on newer KDE versions. Do not disable this unless you are sure that your Quassel will
# run on a matching KDE version only.
set(WITH_OXYGEN AUTO CACHE STRING "Install Oxygen icons (default is \"AUTO\" to install when KDE 4.3 or later is present")

# Enable very experimental support for Qt5. Might break the compilation, or even more!
option(WITH_QT5      "Enable (very) experimental support for Qt5"       OFF)

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

# Qt debug flags
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG QT_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_FULLDEBUG QT_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE QT_NO_DEBUG NDEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO QT_NO_DEBUG NDEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_MINSIZEREL QT_NO_DEBUG NDEBUG)

IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS QT_NO_DEBUG NDEBUG)
ENDIF()

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

# Mac build stuff
if(APPLE AND DEPLOY)
  set(CMAKE_OSX_ARCHITECTURES "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.6")
  set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.6.sdk/")
  add_definitions(-DMAC_10_6_SDK)
endif(APPLE AND DEPLOY)

# Simplify checks
if(WANT_MONO OR WANT_QTCLIENT)
  set(BUILD_GUI true)
endif(WANT_MONO OR WANT_QTCLIENT)
if(WANT_MONO OR WANT_CORE)
  set(BUILD_CORE true)
endif(WANT_MONO OR WANT_CORE)


# Version checks
################

if(WITH_QT5)
  set(QT_MIN_VERSION "5.0.0")
  add_definitions(-DHAVE_QT5)

  # Qt5 needs a new enough cmake...
  cmake_minimum_required(VERSION 2.8.6 FATAL_ERROR)

  # ... for automoc
  set(CMAKE_AUTOMOC ON)
else(WITH_QT5)

  # GUI stuff needs some new features
  if(BUILD_GUI)
    set(QT_MIN_VERSION "4.6.0")
  else(BUILD_GUI)
    set(QT_MIN_VERSION "4.4.0")
  endif(BUILD_GUI)
endif(WITH_QT5)


# Dependencies
##############

if(WITH_QT5)
  find_package(Qt5Core ${QT_MIN_VERSION} REQUIRED)
  # We need QtWidgets
  set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} Widgets)

  # Setup the i18n-related variables
  find_package(Qt5LinguistTools)

else(WITH_QT5)
  # Select a Qt installation here, if you don't want to use system Qt
  if(QT)
    # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
    set(ENV{PATH} ${QT}/bin:$ENV{PATH})
  endif(QT)

  # Now that we have the correct $PATH, lets find Qt!
  find_package(Qt4 ${QT_MIN_VERSION} REQUIRED)
endif(WITH_QT5)


# Neither Qt4 nor Qt5 consider lconvert relevant, so they don't support finding it...
# Rather than shipping hacked buildsys files, let's just infer the path from lrelease
if(QT_LRELEASE_EXECUTABLE)
  get_filename_component(_lrelease_path ${QT_LRELEASE_EXECUTABLE} PATH)
  if(WITH_QT5)
    find_program(QT_LCONVERT_EXECUTABLE NAMES lconvert-qt5 lconvert PATHS ${_lrelease_path} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  else(WITH_QT5)
    find_program(QT_LCONVERT_EXECUTABLE NAMES lconvert-qt4 lconvert PATHS ${_lrelease_path} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  endif(WITH_QT5)
endif(QT_LRELEASE_EXECUTABLE)


# Execinfo is needed for generating backtraces
find_package(ExecInfo)
if(EXECINFO_FOUND)
  add_definitions(-DHAVE_EXECINFO)
  include_directories(${EXECINFO_INCLUDES})
  link_libraries(${EXECINFO_LIBRARIES})
endif(EXECINFO_FOUND)


# PkgConfig isn't strictly required.
# However, some optional deps might not be found if it's not present, so warn!
find_package(PkgConfig)
if(NOT PKG_CONFIG_FOUND)
  message(STATUS "WARNING: PkgConfig not available! Some dependencies for optional features might not be found.")
  message(STATUS "         Affected features might include encryption support, DBus menus and Ayatana notifications.")
endif(NOT PKG_CONFIG_FOUND)


# Setup OpenSSL
# We don't link to or include OpenSSL ourselves, but use exclusively the Qt API.
# Thus, we simply check if OpenSSL support is present in Qt's config and enable our
# own SSL stuff in this case. Qt should care for adding what it needs itself.
if(WITH_OPENSSL)
  if(QT_QCONFIG MATCHES "openssl")
    message(STATUS "Found OpenSSL support in Qt, enabling SSL")
    add_definitions(-DHAVE_SSL)
    set(HAVE_SSL true)
  else(QT_QCONFIG MATCHES "openssl")
    message(STATUS "No OpenSSL support found in Qt, disabling SSL")
    add_definitions(-DQT_NO_OPENSSL)
  endif(QT_QCONFIG MATCHES "openssl")
else(WITH_OPENSSL)
  message(STATUS "Not enabling OpenSSL support")
endif(WITH_OPENSSL)


# Check for GUI specific stuff
if(BUILD_GUI)

  # Setup D-Bus support
  if(WITH_DBUS)
    if(WITH_QT5)
      find_package(Qt5DBus ${QT_MIN_VERSION})
      find_package(Qt5DBusTools)
    endif(WITH_QT5)

    if(QT_QTDBUS_FOUND OR (Qt5DBus_FOUND AND Qt5DBusTools_FOUND))
      message(STATUS "Found QtDBus, enabling D-Bus support")
      add_definitions(-DHAVE_DBUS)
      set(HAVE_DBUS true)
      set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} DBus)

      # check if we have dbusmenu as well
      if(NOT WITH_QT5)
        find_package(DBusMenuQt)
        if(DBUSMENUQT_FOUND)
          message(STATUS "Enabling support for exporting the tray menu via D-Bus")
          add_definitions(-DHAVE_DBUSMENU)
          include_directories(${DBUSMENUQT_INCLUDE_DIR})
          set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} ${DBUSMENUQT_LIBRARIES})
          set(CLIENT_COMPILE_FLAGS "${CLIENT_COMPILE_FLAGS} ${DBUSMENUQT_DEFINITIONS}")
        else(DBUSMENUQT_FOUND)
          message(STATUS "Disabling support for exporting the tray menu via D-Bus")
        endif(DBUSMENUQT_FOUND)
      endif(NOT WITH_QT5)

    else(QT_QTDBUS_FOUND OR (Qt5DBus_FOUND AND Qt5DBusTools_FOUND))
      message(STATUS "QtDBus not found, disabling D-Bus support")
    endif(QT_QTDBUS_FOUND OR (Qt5DBus_FOUND AND Qt5DBusTools_FOUND))
  else(WITH_DBUS)
    message(STATUS "Not enabling D-Bus support")
  endif(WITH_DBUS)

  # Setup QtWebkit support
  if(WITH_WEBKIT)
    if(WITH_QT5)
      find_package(Qt5Webkit ${QT_MIN_VERSION} QUIET)
    endif(WITH_QT5)
    if(QT_QTWEBKIT_FOUND OR Qt5Webkit_FOUND)
      message(STATUS "Found QtWebKit, enabling WebKit support")
      add_definitions(-DHAVE_WEBKIT)
      set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} Webkit XmlPatterns)
      set(HAVE_WEBKIT true)
    else(QT_QTWEBKIT_FOUND OR Qt5Webkit_FOUND)
      message(STATUS "QtWebkit not found, disabling Webkit support")
    endif(QT_QTWEBKIT_FOUND OR Qt5Webkit_FOUND)
  else(WITH_WEBKIT)
    message(STATUS "Not enabling Webkit support")
  endif(WITH_WEBKIT)

  # Setup KDE4 support
  if(WITH_KDE AND NOT WITH_QT5)
    find_package(KDE4)
    if(KDE4_FOUND)
      message(STATUS "Enabling KDE4 integration")
      include_directories(${KDE4_INCLUDES})
      add_definitions(-DHAVE_KDE ${KDE4_DEFINITIONS})
      set(HAVE_KDE 1)
      set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBRARY} ${KDE4_SOLID_LIBS} knotifyconfig)
      # We always use external icons for KDE4 support, since we use its iconloader rather than our own
      set(EMBED_DATA OFF)
    else(KDE4_FOUND)
      message(STATUS "KDE4 not found, disabling KDE integration")
    endif(KDE4_FOUND)
  else(WITH_KDE AND NOT WITH_QT5)
    message(STATUS "Not enabling KDE4 integration")
  endif(WITH_KDE AND NOT WITH_QT5)

  # Setup Phonon support - we only need this if we don't have or want KDE4
  if(NOT HAVE_KDE)
    if(WITH_PHONON)
      if(WITH_QT5)
        find_package(Qt5phonon)
        if(Qt5phonon_FOUND)
          message(STATUS "Enabling Phonon support")
          add_definitions(-DHAVE_PHONON)
          set(HAVE_PHONON true)
          set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} phonon)
        else(Qt5phonon_FOUND)
          message(STATUS "Phonon not found, disabling audio notifications")
        endif(Qt5phonon_FOUND)
      else(WITH_QT5)
        find_package(Phonon)
        if(PHONON_FOUND)
          message(STATUS "Enabling Phonon support")
          add_definitions(-DHAVE_PHONON)
          include_directories(${PHONON_INCLUDES})
          set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} ${PHONON_LIBS})
          set(HAVE_PHONON true)
        else(PHONON_FOUND)
          message(STATUS "Phonon not found, disabling audio notifications")
        endif(PHONON_FOUND)
      endif(WITH_QT5)
    else(WITH_PHONON)
      message(STATUS "Not enabling Phonon support")
    endif(WITH_PHONON)
  endif(NOT HAVE_KDE)

  # Setup libindicate-qt support
  if(WITH_LIBINDICATE AND NOT WITH_QT5)
    pkg_check_modules(INDICATEQT QUIET indicate-qt>=0.2.1)
    if(INDICATEQT_FOUND)
      message(STATUS "Enabling Ayatana notification support")
      set(HAVE_INDICATEQT true)
      add_definitions(-DHAVE_INDICATEQT)
      link_directories(${INDICATEQT_LIBRARY_DIRS})
      set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} ${INDICATEQT_LIBRARIES})
    else(INDICATEQT_FOUND)
      message(STATUS "Disabling Ayatana notification support")
    endif(INDICATEQT_FOUND)
  else(WITH_LIBINDICATE AND NOT WITH_QT5)
    message(STATUS "Not enabling Ayatana notification support")
    # We don't want to link against it even if another package has found it
    set(INDICATEQT_LIBRARIES "")
  endif(WITH_LIBINDICATE AND NOT WITH_QT5)

endif(BUILD_GUI)

# Core-only deps
if(BUILD_CORE)

  # Setup encryption support
  if(WITH_CRYPT AND NOT WITH_QT5)
    find_package(QCA2)
    if(QCA2_FOUND)
      message(STATUS "Enabling encryption support")
      add_definitions(-DHAVE_QCA2)
      set(LINK_QCA2 QCA2)
      set(HAVE_QCA2 true)
      set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_QCA2)
    else(QCA2_FOUND)
      message(STATUS "Disabling encryption support")
    endif(QCA2_FOUND)
  else(WITH_CRYPT AND NOT WITH_QT5)
    message(STATUS "Not enabling encryption support")
  endif(WITH_CRYPT AND NOT WITH_QT5)

  # Setup syslog support
  if(WITH_SYSLOG)
    check_include_file(syslog.h HAVE_SYSLOG_H)
    if(HAVE_SYSLOG_H)
      message(STATUS "Enabling syslog support")
      set(HAVE_SYSLOG true)
      add_definitions(-DHAVE_SYSLOG)
    else(HAVE_SYSLOG_H)
      message(STATUS "Disabling syslog support")
    endif(HAVE_SYSLOG_H)
  else(WITH_SYSLOG)
    message(STATUS "Not enabling syslog support")
  endif(WITH_SYSLOG)

endif(BUILD_CORE)


# Various settings
##################

# needed to compile with mingw without kde
if(MINGW AND NOT HAVE_KDE)
    add_definitions(-D_WIN32_WINNT=0x0500)
    message(STATUS "Added _WIN32_WINNT=0x0500 definition for MinGW")
# workaround for bug in mingw gcc 4.0
    add_definitions(-U__STRICT_ANSI__)
endif(MINGW AND NOT HAVE_KDE)

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
  if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBUGINFO "/debug /INCREMENTAL:YES /NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:YES /NODEFAULTLIB:libcmt")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUGFULL "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    link_libraries(Version dwmapi shlwapi)
  endif(MSVC)
  if(HAVE_SSL AND STATIC)
     find_package(OpenSSL REQUIRED)
     link_libraries(${OPENSSL_LIBRARIES} ${OPENSSL_EAY_LIBRARIES})
  endif(HAVE_SSL AND STATIC)
endif(WIN32)

if(HAVE_INDICATEQT)
  add_definitions(-DXDG_APPS_INSTALL_DIR=${XDG_APPS_INSTALL_DIR})
endif(HAVE_INDICATEQT)

if(NOT WIN32)
  check_function_exists(umask HAVE_UMASK)
  if(HAVE_UMASK)
    add_definitions(-DHAVE_UMASK)
  endif(HAVE_UMASK)
endif(NOT WIN32)

# We need to create a version.gen
# For this, we create our genversion binary and make sure it is run every time.

setup_qt_variables()
include_directories(${QUASSEL_QT_INCLUDES})

add_executable(genversion ${CMAKE_SOURCE_DIR}/src/common/genversion.cpp)
target_link_libraries(genversion ${QUASSEL_QT_LIBRARIES})
set_target_properties(genversion PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

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

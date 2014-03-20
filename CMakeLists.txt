# Main CMake file for building Quassel IRC
#
# See INSTALL for possible CMake options (or read the code, Luke)
#####################################################################

# General setup
#####################################################################

cmake_minimum_required(VERSION 2.8.9)
project(QuasselIRC)


# Versions
set(QUASSEL_MAJOR  0)
set(QUASSEL_MINOR 11)
set(QUASSEL_PATCH  0)
set(QUASSEL_VERSION_STRING "0.11-pre")


# Tell CMake about or own modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# General conveniences
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


# Moar stuff
include(CheckFunctionExists)
include(CheckIncludeFile)

include(QuasselCompileSettings)
include(QuasselMacros)

include(CMakeDependentOption)


# Options and variables that can be set on the command line
#####################################################################


# First, choose a Qt version. We support USE_QT4 and USE_QT5; if neither is set, prefer Qt4 for now
option(USE_QT5 "Enable support for Qt5 (disables KDE integration)" OFF)
if (USE_QT4) # takes precedence
    set(USE_QT5 OFF)
else()
    if (NOT USE_QT5)
        set(USE_QT4 ON)
    endif()
endif()

# Select the binaries to build
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the Qt4 GUI client binary"          ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" ON)


# Whether to enable KDE integration (pulls in kdelibs and friends as a dependency); requires Qt4 for now
cmake_dependent_option(WITH_KDE      "KDE4 integration" OFF "USE_QT4" OFF)

# Some options don't make sense with KDE
cmake_dependent_option(WITH_PHONON   "Phonon support (for audio notifications)"           ON "NOT WITH_KDE" OFF)
cmake_dependent_option(WITH_SNORE    "Snore notification support"                        OFF "NOT WITH_KDE" OFF)
cmake_dependent_option(WITH_OXYGEN   "Install Oxygen icon set (usually shipped with KDE)" ON "NOT WITH_KDE" OFF)

# Misc features
option(WITH_OPENSSL     "OpenSSL support (secure networking)"             ON)
option(WITH_DBUS        "Use D-Bus for the tray icon (StatusNotifier)"    ON)
option(WITH_WEBKIT      "WebKit support (for link previews)"              ON)
option(WITH_CRYPT       "Encryption support (QCA)"                        ON)
option(WITH_LIBINDICATE "Ayatana (libindicate) notification support"      ON)
cmake_dependent_option(WITH_SYSLOG   "Support logging to syslog"          ON "NOT WIN32" OFF)

if (APPLE)
    # Notification Center is only available in > 10.8, which is Darwin v12
    if (CMAKE_SYSTEM_VERSION VERSION_GREATER "11.9.9")
        option(WITH_NOTIFICATION_CENTER "OS X Notification Center support" ON)
    endif()
endif()

# Always embed on Windows or for a static build; never embed when enabling KDE integration
set(EMBED_DEFAULT OFF)
if (STATIC OR WIN32)
    set(EMBED_DEFAULT ON)
endif()
cmake_dependent_option(EMBED_DATA "Embed icons and translations in the binaries instead of installing them" ${EMBED_DEFAULT}
                                   "NOT STATIC;NOT WIN32;NOT WITH_KDE" ${EMBED_DEFAULT})

cmake_dependent_option(DEPLOY      "Add required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF "APPLE" OFF)

# Handle with care
set(QT_PATH "" CACHE PATH "Path to a Qt4 installation to use instead of the system Qt (e.g. for static builds)")


# Static builds are not supported and require some manual setup! Don't enable unless you know what you're doing (we don't know either)
cmake_dependent_option(STATIC      "Enable static building (not supported)" OFF "NOT WITH_KDE" OFF)

# For static builds, arbitrary extra libs might need to be linked
# Define a comma-separated list here
# e.g. for pgsql, we need -DLINK_EXTRA=pq;crypt
set(LINK_EXTRA "" CACHE STRING "Semicolon-separated list of libraries to be linked")
if (LINK_EXTRA)
    string(REPLACE "," ";" LINK_EXTRA ${LINK_EXTRA})
    link_libraries(${LINK_EXTRA})
endif()


# Simplify later checks
if (WANT_MONO OR WANT_QTCLIENT)
    set(BUILD_GUI true)
endif()
if (WANT_MONO OR WANT_CORE)
    set(BUILD_CORE true)
endif()


# Set up Qt
#####################################################################


if (USE_QT5)
    message(STATUS "Building with the Qt5 libraries...")
    set(QT_MIN_VERSION "5.2.0")
    add_definitions(-DHAVE_QT5)
else()
    message(STATUS "Building with the Qt4 libraries...")
    if (BUILD_GUI)
        set(QT_MIN_VERSION "4.6.0")
    else()
        set(QT_MIN_VERSION "4.4.0")
    endif()
endif()


if (USE_QT5)
    find_package(Qt5Core ${QT_MIN_VERSION} REQUIRED)
    # We need QtWidgets
    set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} Widgets)

    # Setup the i18n-related variables
    find_package(Qt5LinguistTools)

else()
    # Select a Qt installation here, if you don't want to use system Qt
    if(QT_PATH)
        # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
        set(ENV{PATH} ${QT}/bin:$ENV{PATH})
    endif()

    # Now that we have the correct $PATH, lets find Qt!
    find_package(Qt4 ${QT_MIN_VERSION} REQUIRED)
endif()


# Neither Qt4 nor Qt5 consider lconvert relevant, so they don't support finding it...
# Rather than shipping hacked buildsys files, let's just infer the path from lrelease
if (QT_LRELEASE_EXECUTABLE)
    get_filename_component(_lrelease_path ${QT_LRELEASE_EXECUTABLE} PATH)
    if (USE_QT5)
        find_program(QT_LCONVERT_EXECUTABLE NAMES lconvert-qt5 lconvert PATHS ${_lrelease_path} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    else()
        find_program(QT_LCONVERT_EXECUTABLE NAMES lconvert-qt4 lconvert PATHS ${_lrelease_path} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    endif()
endif()


# Dependencies
#####################################################################

# zlib for compression, however we can always fall back to miniz
find_package(ZLIB)
if(ZLIB_FOUND)
  message(STATUS "Using system zlib for compression")
  add_definitions(-DHAVE_ZLIB)
  include_directories(${ZLIB_INCLUDE_DIRS})
  set(COMMON_LIBRARIES ${COMMON_LIBRARIES} ${ZLIB_LIBRARIES})
else()
  message(STATUS "zlib NOT found, using bundled miniz for compression")
  if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    message(STATUS "WARNING: This may be slow on 32 bit systems!")
  endif()
endif()


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
    if (USE_QT5)
      find_package(Qt5DBus ${QT_MIN_VERSION})
      find_package(Qt5DBusTools)
    endif()

    if(QT_QTDBUS_FOUND OR (Qt5DBus_FOUND AND Qt5DBusTools_FOUND))
      message(STATUS "Found QtDBus, enabling D-Bus support")
      add_definitions(-DHAVE_DBUS)
      set(HAVE_DBUS true)
      set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} DBus)

      # check if we have dbusmenu as well
      if (NOT USE_QT5)
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
      endif()

    else(QT_QTDBUS_FOUND OR (Qt5DBus_FOUND AND Qt5DBusTools_FOUND))
      message(STATUS "QtDBus not found, disabling D-Bus support")
    endif(QT_QTDBUS_FOUND OR (Qt5DBus_FOUND AND Qt5DBusTools_FOUND))
  else(WITH_DBUS)
    message(STATUS "Not enabling D-Bus support")
  endif(WITH_DBUS)

  # Setup QtWebkit support
  if(WITH_WEBKIT)
    if (USE_QT5)
      find_package(Qt5Webkit ${QT_MIN_VERSION} QUIET)
    endif()
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
  if(WITH_KDE AND NOT USE_QT5)
    # KDE has overzealous CFLAGS making miniz not compile, so save our old flags
    set(_cflags ${CMAKE_C_FLAGS})
    find_package(KDE4)
    if(KDE4_FOUND)
      message(STATUS "Enabling KDE4 integration")
      include_directories(${KDE4_INCLUDES})
      add_definitions(-DHAVE_KDE ${KDE4_DEFINITIONS})
      set(HAVE_KDE 1)
      set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBRARY} ${KDE4_SOLID_LIBS} ${KDE4_KNOTIFYCONFIG_LIBRARY})
      # We always use external icons for KDE4 support, since we use its iconloader rather than our own
      set(EMBED_DATA OFF)
      # Restore our old CFLAGS
      set(CMAKE_C_FLAGS ${_cflags})
    else(KDE4_FOUND)
      message(STATUS "KDE4 not found, disabling KDE integration")
    endif(KDE4_FOUND)
  else(WITH_KDE AND NOT USE_QT5)
    message(STATUS "Not enabling KDE4 integration")
  endif(WITH_KDE AND NOT USE_QT5)

  # Setup Phonon support - we only need this if we don't have or want KDE4
  if(NOT HAVE_KDE)
    if(WITH_PHONON)
      if(USE_QT5)
        find_package(Qt5phonon)
        if(Qt5phonon_FOUND)
          message(STATUS "Enabling Phonon support")
          add_definitions(-DHAVE_PHONON)
          set(HAVE_PHONON true)
          set(CLIENT_QT_MODULES ${CLIENT_QT_MODULES} phonon)
        else(Qt5phonon_FOUND)
          message(STATUS "Phonon not found, disabling audio notifications")
        endif(Qt5phonon_FOUND)
      else(USE_QT5)
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
      endif(USE_QT5)
    else(WITH_PHONON)
      message(STATUS "Not enabling Phonon support")
    endif(WITH_PHONON)

    find_package(Libsnore)
    if(LIBSNORE_FOUND)
        add_definitions(-DHAVE_LIBSNORE)
        set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} ${LIBSNORE_LIBRARIES})
        set(HAVE_SNORENOTIFY true)
    endif(LIBSNORE_FOUND)
  endif(NOT HAVE_KDE)

  # Setup libindicate-qt support
  if(WITH_LIBINDICATE AND NOT USE_QT5)
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
  else(WITH_LIBINDICATE AND NOT USE_QT5)
    message(STATUS "Not enabling Ayatana notification support")
    # We don't want to link against it even if another package has found it
    set(INDICATEQT_LIBRARIES "")
  endif(WITH_LIBINDICATE AND NOT USE_QT5)

  # Setup OS X notification center support
  if(WITH_NOTIFICATION_CENTER AND APPLE)
    set(HAVE_NOTIFICATION_CENTER true)
    add_definitions(-DHAVE_NOTIFICATION_CENTER)
    set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES}
      /System/Library/Frameworks/Foundation.framework
    )
  endif()
endif(BUILD_GUI)

# Core-only deps
if(BUILD_CORE)

  # Setup encryption support
  if(WITH_CRYPT AND NOT USE_QT5)
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
  else(WITH_CRYPT AND NOT USE_QT5)
    message(STATUS "Not enabling encryption support")
  endif(WITH_CRYPT AND NOT USE_QT5)

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

# Generate version information from Git
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_HEAD)
git_describe(GIT_DESCRIBE --long)

# Sanitize things if we're not in a Git repo
if(NOT GIT_HEAD OR NOT GIT_DESCRIBE)
    set(GIT_HEAD "")
    set(GIT_DESCRIBE "")
endif()

configure_file(version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)

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

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
include(FeatureSummary)


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
option(WANT_QTCLIENT "Build the Qt GUI client binary"           ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" ON)

add_feature_info(WANT_CORE WANT_CORE "Build the core (server) binary")
add_feature_info(WANT_QTCLIENT WANT_QTCLIENT "Build the Qt GUI client binary")
add_feature_info(WANT_MONO WANT_MONO "Build the monolithic (all-in-one) binary")


# Whether to enable KDE integration (pulls in kdelibs and friends as a dependency); requires Qt4 for now
cmake_dependent_option(WITH_KDE      "KDE4 integration" OFF "USE_QT4" OFF)
add_feature_info(WITH_KDE WITH_KDE "Enable KDE4 integration")

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
        add_feature_info(WITH_NOTIFICATION_CENTER WITH_NOTIFICATION_CENTER "Use the OS X Notification Center")
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
#####################################################################

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
    find_package(Qt5Core ${QT_MIN_VERSION} QUIET REQUIRED)

    # Contains lconvert and friends
    find_package(Qt5LinguistTools QUIET)
    set_package_properties(Qt5LinguistTools PROPERTIES TYPE RECOMMENDED
                           PURPOSE "Required for localization support"
    )
else()
    # Select a Qt installation here, if you don't want to use system Qt
    if(QT_PATH)
        # FindQt4 will look for the qmake binary in $PATH, so we just prepend QT_PATH
        set(ENV{PATH} ${QT_PATH}/bin:$ENV{PATH})
    endif()

    # Now that we have the correct $PATH, lets find Qt!
    find_package(Qt4 ${QT_MIN_VERSION} QUIET REQUIRED)
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

add_feature_info("Qt Linguist Tools" QT_LRELEASE_EXECUTABLE "Translation support for Quassel")

# Optional Dependencies
#
# Note that you can forcefully disable optional packages
# using -DCMAKE_DISABLE_FIND_PACKAGE_<PkgName>=TRUE
#####################################################################

if(USE_QT5)
    if (BUILD_GUI)
        find_package(Qt5DBus QUIET)
        set_package_properties(Qt5DBus PROPERTIES TYPE RECOMMENDED
            URL "http://qt.digia.com"
            DESCRIPTION "D-Bus support for Qt5"
            PURPOSE     "Needed for supporting D-Bus-based notifications and tray icon, used by most modern desktop environments"
        )
        if (Qt5DBus_FOUND)
            find_package(dbusmenu-qt5 QUIET)
            set_package_properties(dbusmenu-qt5 PROPERTIES TYPE RECOMMENDED
                URL "https://launchpad.net/libdbusmenu-qt"
                DESCRIPTION "a library implementing the DBusMenu specification"
                PURPOSE     "Required for having a context menu for the D-Bus-based tray icon"
            )
        endif()

        find_package(Phonon4Qt5 QUIET)
        set_package_properties(Phonon4Qt5 PROPERTIES TYPE RECOMMENDED
            URL "https://projects.kde.org/projects/kdesupport/phonon"
            DESCRIPTION "a multimedia abstraction library"
            PURPOSE     "Required for audio notifications"
        )

        if (WITH_WEBKIT)
            find_package(Qt5Webkit QUIET)
            set_package_properties(Qt5Webkit PROPERTIES TYPE RECOMMENDED
                URL "http://qt.digia.com"
                DESCRIPTION "a Webkit implementation for Qt"
                PURPOSE     "Needed for displaying previews for URLs in chat"
            )
        endif()
        add_feature_info("WITH_WEBKIT and QtWebkit module" Qt5Webkit_FOUND "Support showing previews for URLs in chat")

    endif(BUILD_GUI)
    if (BUILD_CORE)
        # While QCA2 seems to support Qt5, it is not actually co-installable or distinguishable from the Qt4 version...
        # In order to avoid linking against the Qt4 version (which is probably the one installed), disable this for now
        #find_package(QCA2 QUIET)
        #set_package_properties(QCA2 PROPERTIES TYPE RECOMMENDED
        #    URL "https://projects.kde.org/projects/kdesupport/qca"
        #    DESCRIPTION "Qt Cryptographic Architecture"
        #    PURPOSE "Required for encryption support"
        #)

    endif(BUILD_CORE)

else(USE_QT5)
    if (BUILD_GUI)
        add_feature_info("QtDBus module" QT_QTDBUS_FOUND "Needed for supporting D-Bus-based notifications and tray icon, used by most modern desktop environments")
        if (QT_QTDBUS_FOUND)
            find_package(libdbusmenu-qt QUIET)
            set_package_properties(dbusmenu-qt PROPERTIES TYPE RECOMMENDED
                URL "https://launchpad.net/libdbusmenu-qt"
                DESCRIPTION "a library implementing the DBusMenu specification"
                PURPOSE     "Required for having a context menu for the D-Bus-based tray icon"
            )
        endif()

        if (WITH_WEBKIT AND QT_QTWEBKIT_FOUND)
            set(HAVE_WEBKIT true)
        endif()
        add_feature_info("WITH_WEBKIT and QtWebkit module" HAVE_WEBKIT "Support showing previews for URLs in chat")

        if (WITH_KDE)
            # KDE has overzealous CFLAGS making miniz not compile, so save our old flags
            set(_cflags ${CMAKE_C_FLAGS})
            find_package(KDE4 4.4 QUIET)
            set_package_properties(KDE4 PROPERTIES TYPE REQUIRED
                URL "http://www.kde.org"
                DESCRIPTION "a world-class desktop environment"
                PURPOSE "Enables various bits for improving integration with KDE"
            )
            set(CMAKE_C_FLAGS ${_cflags})

        else(WITH_KDE)
            find_package(Phonon QUIET)
            set_package_properties(Phonon PROPERTIES TYPE RECOMMENDED
                URL "https://projects.kde.org/projects/kdesupport/phonon"
                DESCRIPTION "a multimedia abstraction library"
                PURPOSE     "Required for audio notifications"
            )

            find_package(Libsnore QUIET)
            set_package_properties(Libsnore PROPERTIES TYPE OPTIONAL
                URL "https://github.com/TheOneRing/Snorenotify"
                DESCRIPTION "a cross-platform notification framework"
                PURPOSE     "Enable support for the snorenotify framework"
            )
        endif(WITH_KDE)

        find_package(IndicateQt QUIET)
        set_package_properties(IndicateQt PROPERTIES TYPE OPTIONAL
            URL "https://launchpad.net/libindicate-qt/"
            DESCRIPTION "a library to raise flags on DBus for other components of the desktop to pick up and visualize"
            PURPOSE     "Provides integration into the Ayatana notification system used by e.g. Ubuntu"
        )

    endif(BUILD_GUI)
    if (BUILD_CORE)

        find_package(QCA2 QUIET)
        set_package_properties(QCA2 PROPERTIES TYPE RECOMMENDED
            URL "https://projects.kde.org/projects/kdesupport/qca"
            DESCRIPTION "Qt Cryptographic Architecture"
            PURPOSE     "Required for encryption support"
        )


    endif()
endif()

# Non-Qt-based packages

# zlib for compression, however we can always fall back to miniz
find_package(ZLIB QUIET)
set_package_properties(ZLIB PROPERTIES TYPE RECOMMENDED
    URL "http://www.zlib.net"
    DESCRIPTION "a popular compression library"
    PURPOSE     "Use the most common library for protocol compression, instead of the bundled miniz implementation"
)


if (NOT WIN32)
    # Execinfo is needed for generating backtraces
    find_package(ExecInfo QUIET)
    set_package_properties(ExecInfo PROPERTIES TYPE OPTIONAL
        DESCRIPTION "a library for inspecting backtraces"
        PURPOSE "Used for generating backtraces in case of a crash"
    )
endif()


# Various checks
#####################################################################

if (NOT ZLIB_FOUND)
    message(STATUS "zlib NOT found, using bundled miniz for compression")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        message(STATUS "WARNING: This may be slow on 32 bit systems!")
    endif()
endif()

if (KDE4_FOUND)
    # We always use external icons for KDE4 support, since we use its iconloader rather than our own
    set(EMBED_DATA OFF)

    # Better have the compile flags global, even for the core, to avoid problems with linking the mono client
    add_definitions(-DHAVE_KDE ${KDE4_DEFINITIONS})
endif()

# Check for SSL support in Qt (broken for Qt5 currently)
# We don't link to the OpenSSL libraries ourselves.
if (QT_QCONFIG MATCHES "openssl")
    set(HAVE_SSL true)
    add_definitions(-DHAVE_SSL)
endif()
add_feature_info("SSL support in Qt" HAVE_SSL "Use secure network connections")

# Check for syslog support
if (NOT WIN32)
    check_include_file(syslog.h HAVE_SYSLOG)
    add_feature_info("syslog.h" HAVE_SYSLOG "Provide support for logging to the syslog")
endif()


# Various settings
##################

# needed to compile with mingw without kde
if (MINGW AND NOT KDE4_FOUND)
    add_definitions(-D_WIN32_WINNT=0x0500)
    message(STATUS "Added _WIN32_WINNT=0x0500 definition for MinGW")
# workaround for bug in mingw gcc 4.0
    add_definitions(-U__STRICT_ANSI__)
endif()

# Now set up install locations; those are set by KDE if integration is enabled
if(NOT KDE4_FOUND)
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
endif()

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

if(INDICATEQT_FOUND)
  add_definitions(-DXDG_APPS_INSTALL_DIR=${XDG_APPS_INSTALL_DIR})
endif()

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

# Set up and display feature summary
#####################################################################

feature_summary(WHAT ALL
                INCLUDE_QUIET_PACKAGES
                FATAL_ON_MISSING_REQUIRED_PACKAGES
)

# Main CMake file for building Quassel IRC
#
# See INSTALL for possible CMake options (or read the code, Luke)
#####################################################################

# General setup
#####################################################################

project(QuasselIRC)

# Versions
set(QUASSEL_MAJOR  0)
set(QUASSEL_MINOR 13)
set(QUASSEL_PATCH  0)
set(QUASSEL_VERSION_STRING "0.13-pre")

# Output CMake version and build type for debug reasons
message(STATUS "Using CMake ${CMAKE_VERSION}")
message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

# Tell CMake about or own modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# General conveniences
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include various CMake modules...
include(CMakePushCheckState)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCXXSourceCompiles)
include(CMakeDependentOption)
include(FeatureSummary)

# ... and our own stuff
include(QuasselCompileSettings)
include(QuasselMacros)


# Options and variables that can be set on the command line
#####################################################################

# First, choose a Qt version. We support USE_QT5 and USE_QT4; if neither is set, Qt5 will be used
option(USE_QT5 "Enable support for Qt5" OFF)
if (USE_QT5) # takes precedence
    set(USE_QT4 OFF)
else()
    if (NOT USE_QT4)
        set(USE_QT5 ON)
    endif()
endif()

# Select the binaries to build
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the client-only binary"             ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" ON)
add_feature_info(WANT_CORE WANT_CORE "Build the core (server) binary")
add_feature_info(WANT_QTCLIENT WANT_QTCLIENT "Build the client-only binary (requires a core to connect to)")
add_feature_info(WANT_MONO WANT_MONO "Build the monolithic (all-in-one) binary")

# Whether to enable KDE integration (work in progress for Qt5 / KDE Frameworks)
# Note that when building with Qt5, WITH_KDE enables integration with higher-tier KDE frameworks that
# require runtime support. We still optionally make use of certain Tier 1 frameworks even if WITH_KDE
# is disabled.
if (USE_QT4)
    option(WITH_KDE "KDE4 integration" OFF)
    add_feature_info(WITH_KDE WITH_KDE "Enable KDE4 integration")
else()
    option(WITH_KDE "Integration with the KDE Frameworks runtime environment")
    add_feature_info(WITH_KDE WITH_KDE "Integrate with the KDE Frameworks runtime environment")
endif()

cmake_dependent_option(WITH_BREEZE "Install Breeze icon theme (usually shipped with KDE)" ON "NOT WITH_KDE" OFF)
cmake_dependent_option(WITH_BREEZE_DARK "Install Dark Breeze icon theme (usually shipped with KDE)" ON "NOT WITH_KDE" OFF)
cmake_dependent_option(WITH_OXYGEN "Install Oxygen icon theme (usually shipped with KDE)" OFF "NOT WITH_KDE" OFF)
if (NOT WITH_KDE)
    add_feature_info(WITH_BREEZE WITH_BREEZE "Install Breeze icon theme")
    add_feature_info(WITH_BREEZE_DARK WITH_BREEZE_DARK "Install Dark Breeze icon theme")
    add_feature_info(WITH_OXYGEN WITH_OXYGEN "Install Oxygen icon theme")
endif()

if (WITH_OXYGEN)
    add_definitions(-DWITH_OXYGEN)
endif()
if (WITH_BREEZE)
    add_definitions(-DWITH_BREEZE)
endif()
if (WITH_BREEZE_DARK)
    add_definitions(-DWITH_BREEZE_DARK)
endif()

# For this, the feature info is added after we know if QtWebkit is installed
option(WITH_WEBKIT "WebKit support (for link previews) (legacy)" OFF)

# For this, the feature info is added after we know if QtWebEngine is installed
option(WITH_WEBENGINE "WebEngine support (for link previews)" ON)

if (APPLE)
    # Notification Center is only available in > 10.8, which is Darwin v12
    if (NOT CMAKE_SYSTEM_VERSION VERSION_LESS 12)
        option(WITH_NOTIFICATION_CENTER "OS X Notification Center support" ON)
        add_feature_info(WITH_NOTIFICATION_CENTER WITH_NOTIFICATION_CENTER "Use the OS X Notification Center")
    endif()
    find_library(CARBON_LIBRARY Carbon)
    mark_as_advanced(CARBON_LIBRARY)
    link_libraries(${CARBON_LIBRARY})
endif()

# Always embed on Windows, OSX or for a static build; never embed when enabling KDE integration
set(EMBED_DEFAULT OFF)
if (STATIC OR WIN32 OR APPLE)
    set(EMBED_DEFAULT ON)
endif()
cmake_dependent_option(EMBED_DATA "Embed icons and translations into the binaries instead of installing them" ${EMBED_DEFAULT}
                                   "NOT STATIC;NOT WIN32;NOT WITH_KDE" ${EMBED_DEFAULT})
if (NOT EMBED_DEFAULT)
    add_feature_info(EMBED_DATA EMBED_DATA "Embed icons and translations in the binaries instead of installing them")
endif()

# The following options are not for end-user consumption, so don't list them in the feature summary
cmake_dependent_option(DEPLOY "Add required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF "APPLE" OFF)

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


# List of authenticators and the cmake flags to build them
# (currently that's just LDAP, but more can be added here).
####################################################################
option(WITH_LDAP "Enable LDAP authentication support if present on system" ON)

# Setup CMake
#####################################################################

if (USE_QT5 AND WITH_KDE)
    cmake_minimum_required(VERSION 2.8.12)
else()
    cmake_minimum_required(VERSION 2.8.9)
endif()

# Setting COMPILE_DEFINITIONS_<CONFIG> is deprecated since CMake 3.0 in favor of generator expressions.
# These have existed since CMake 2.8.10; until we depend on that, we have to explicitly enable the old policy.
if (POLICY CMP0043)
    cmake_policy(SET CMP0043 OLD)
endif()

# Honor visibility settings for all target types
if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

# Don't automoc generated files
if (POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
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

# Find package dependencies
#
# Note that you can forcefully disable optional packages
# using -DCMAKE_DISABLE_FIND_PACKAGE_<PkgName>=TRUE
#####################################################################

if (USE_QT5)
    message(STATUS "Building for Qt5...")
    set(QT_MIN_VERSION "5.2.0")
    add_definitions(-DHAVE_QT5)

    find_package(Qt5Core ${QT_MIN_VERSION} QUIET)
    set_package_properties(Qt5Core PROPERTIES TYPE REQUIRED
        URL "https://www.qt.io/"
        DESCRIPTION "contains core functionality for Qt"
    )
    # find_package without REQUIRED won't check for the version properly; also, older Qt5 versions
    # used Qt5Core_VERSION_STRING... let's just make sure here that we bail out here if our Qt5 is not new enough.
    if (NOT Qt5Core_VERSION OR Qt5Core_VERSION VERSION_LESS ${QT_MIN_VERSION})
        message(FATAL_ERROR "Could NOT find Qt5 >= version ${QT_MIN_VERSION}!")
    endif()

    find_package(Qt5Network QUIET)
    set_package_properties(Qt5Network PROPERTIES TYPE REQUIRED
        DESCRIPTION "the network module for Qt5"
    )

    if (BUILD_GUI)
        find_package(Qt5Gui QUIET)
        set_package_properties(Qt5Gui PROPERTIES TYPE REQUIRED
            DESCRIPTION "the GUI module for Qt5"
        )
        find_package(Qt5Widgets QUIET)
        set_package_properties(Qt5Widgets PROPERTIES TYPE REQUIRED
            DESCRIPTION "the widgets module for Qt5"
        )

        if (NOT WIN32)
            find_package(Qt5DBus QUIET)
            set_package_properties(Qt5DBus PROPERTIES TYPE RECOMMENDED
                URL "https://www.qt.io/"
                DESCRIPTION "D-Bus support for Qt5"
                PURPOSE     "Needed for supporting D-Bus-based notifications and tray icon, used by most modern desktop environments"
            )
            if (Qt5DBus_FOUND)
                find_package(dbusmenu-qt5 QUIET CONFIG)
                set_package_properties(dbusmenu-qt5 PROPERTIES TYPE RECOMMENDED
                    URL "https://launchpad.net/libdbusmenu-qt"
                    DESCRIPTION "a library implementing the DBusMenu specification"
                    PURPOSE     "Required for having a context menu for the D-Bus-based tray icon"
                )
            endif()
        endif()

        find_package(Qt5Multimedia QUIET)
        set_package_properties(Qt5Multimedia PROPERTIES TYPE RECOMMENDED
            URL "https://www.qt.io/"
            DESCRIPTION "Multimedia support for Qt5"
            PURPOSE     "Required for audio notifications"
        )

        if (NOT Qt5Multimedia_FOUND)
            find_package(Phonon4Qt5 QUIET)
            set_package_properties(Phonon4Qt5 PROPERTIES TYPE RECOMMENDED
                URL "https://projects.kde.org/projects/kdesupport/phonon"
                DESCRIPTION "a multimedia abstraction library"
                PURPOSE     "Required for audio notifications"
            )
        endif()

        find_package(LibsnoreQt5 0.7.0 QUIET)
        set_package_properties(LibsnoreQt5 PROPERTIES TYPE OPTIONAL
            URL "https://projects.kde.org/projects/playground/libs/snorenotify"
            DESCRIPTION "a cross-platform notification framework"
            PURPOSE     "Enable support for the snorenotify framework"
        )
        if(LibsnoreQt5_FOUND)
            find_package(LibsnoreSettingsQt5)
            set_package_properties(LibsnoreSettingsQt5 PROPERTIES TYPE REQUIRED
                URL "https://projects.kde.org/projects/playground/libs/snorenotify"
                DESCRIPTION "a cross-platform notification framework"
                PURPOSE     "Enable support for the snorenotify framework"
            )
        endif()


        if (WITH_WEBKIT)
            find_package(Qt5WebKit QUIET)
            set_package_properties(Qt5WebKit PROPERTIES TYPE RECOMMENDED
                URL "https://www.qt.io/"
                DESCRIPTION "a WebKit implementation for Qt"
                PURPOSE     "Needed for displaying previews for URLs in chat"
            )
            if (Qt5WebKit_FOUND)
                find_package(Qt5WebKitWidgets QUIET)
                set_package_properties(Qt5WebKitWidgets PROPERTIES TYPE RECOMMENDED
                    URL "https://www.qt.io/"
                    DESCRIPTION "widgets for Qt's WebKit implementation"
                    PURPOSE     "Needed for displaying previews for URLs in chat"
                )
            endif()
        endif()

        if (WITH_WEBKIT AND Qt5WebKitWidgets_FOUND)
            set(HAVE_WEBKIT true)
        endif()
        add_feature_info("WITH_WEBKIT, QtWebKit and QtWebKitWidgets modules" HAVE_WEBKIT "Support showing previews for URLs in chat (legacy)")

        if (WITH_WEBENGINE)
            find_package(Qt5WebEngine QUIET)
            set_package_properties(Qt5WebEngine PROPERTIES TYPE RECOMMENDED
                URL "https://www.qt.io/"
                DESCRIPTION "a WebEngine implementation for Qt"
                PURPOSE     "Needed for displaying previews for URLs in chat"
            )
            if (Qt5WebEngine_FOUND)
                find_package(Qt5WebEngineWidgets QUIET)
                set_package_properties(Qt5WebEngineWidgets PROPERTIES TYPE RECOMMENDED
                    URL "https://www.qt.io/"
                    DESCRIPTION "widgets for Qt's WebEngine implementation"
                    PURPOSE     "Needed for displaying previews for URLs in chat"
                )
            endif()
        endif()

        if (WITH_WEBENGINE AND Qt5WebEngineWidgets_FOUND)
            set(HAVE_WEBENGINE true)
        endif()
        add_feature_info("WITH_WEBENGINE, QtWebEngine and QtWebEngineWidgets modules" HAVE_WEBENGINE "Support showing previews for URLs in chat")

        # KDE Frameworks
        ################

        if (WITH_KDE)
            set(ecm_find_type "REQUIRED")
        else()
            # Even with KDE integration disabled, we optionally use tier1 frameworks if we find them
            set(ecm_find_type "RECOMMENDED")
        endif()

        # extra-cmake-modules
        find_package(ECM NO_MODULE QUIET)
        set_package_properties(ECM PROPERTIES TYPE ${ecm_find_type}
            URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules"
            DESCRIPTION "extra modules for CMake, maintained by the KDE project"
            PURPOSE     "Required to find KDE Frameworks components"
        )

        if (ECM_FOUND)
            list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
            if (WITH_KDE)
                find_package(KF5 COMPONENTS ConfigWidgets CoreAddons Notifications NotifyConfig TextWidgets WidgetsAddons XmlGui QUIET)
                set_package_properties(KF5 PROPERTIES TYPE REQUIRED
                    URL "http://www.kde.org"
                    DESCRIPTION "KDE Frameworks"
                    PURPOSE     "Required for integration into the Plasma desktop"
                )
            else()
                find_package(KF5Sonnet QUIET)
                set_package_properties(KF5Sonnet PROPERTIES TYPE RECOMMENDED
                    URL "http://api.kde.org/frameworks-api/frameworks5-apidocs/sonnet/html"
                    DESCRIPTION "framework for providing spell-checking capabilities"
                    PURPOSE "Enables spell-checking support in input widgets"
                )
            endif()
        endif()

    endif()

    if (BUILD_CORE)
        find_package(Qt5Script QUIET)
        set_package_properties(Qt5Script PROPERTIES TYPE REQUIRED
            DESCRIPTION "provides scripting support for Qt5"
        )
        find_package(Qt5Sql QUIET)
        set_package_properties(Qt5Sql PROPERTIES TYPE REQUIRED
            DESCRIPTION "the database support module for Qt5"
        )

        find_package(QCA2-QT5)
        set_package_properties(QCA2-QT5 PROPERTIES TYPE RECOMMENDED
            URL "https://projects.kde.org/projects/kdesupport/qca"
            DESCRIPTION "Qt Cryptographic Architecture"
            PURPOSE "Required for encryption support"
        )

    endif()

    find_package(Qt5LinguistTools QUIET)
    set_package_properties(Qt5LinguistTools PROPERTIES TYPE RECOMMENDED
                           DESCRIPTION "contains tools for handling translation files"
                           PURPOSE "Required for having translations"
    )

    # Some Qt5 versions do not define a target for lconvert, so we need to find it ourselves
    if (Qt5LinguistTools_FOUND)
        if (NOT TARGET Qt5::lconvert AND TARGET Qt5::lrelease)
            get_target_property(_lrelease_location Qt5::lrelease LOCATION)
            get_filename_component(_lrelease_path ${_lrelease_location} PATH)
            find_program(QT_LCONVERT_EXECUTABLE NAMES lconvert-qt5 lconvert PATHS ${_lrelease_path} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
        elseif(TARGET Qt5::lconvert AND NOT Qt5_LCONVERT_EXECUTABLE)
            # Newer Qt5 versions define the target, but not the Qt5_LCONVERT_EXECUTABLE variable for some reason
            get_target_property(QT_LCONVERT_EXECUTABLE Qt5::lconvert LOCATION)
        endif()

        # Compatibility with the Qt4 variables
        set(QT_LRELEASE_EXECUTABLE ${Qt5_LRELEASE_EXECUTABLE})
        set(QT_LUPDATE_EXECUTABLE ${Qt5_LUPDATE_EXECUTABLE})
        if (Qt5_LCONVERT_EXECUTABLE)
            set(QT_LCONVERT_EXECUTABLE ${Qt5_LCONVERT_EXECUTABLE})
        endif()
    endif()

else()
    message(STATUS "Building for Qt4...")
    set(QT_MIN_VERSION "4.8.0")

    find_package(Qt4 ${QT_MIN_VERSION} QUIET REQUIRED)

    if (BUILD_GUI)
        add_feature_info("QtDBus module" QT_QTDBUS_FOUND "Needed for supporting D-Bus-based notifications and tray icon, used by most modern desktop environments")
        if (QT_QTDBUS_FOUND)
            find_package(dbusmenu-qt QUIET CONFIG)
            set_package_properties(dbusmenu-qt PROPERTIES TYPE RECOMMENDED
                URL "https://launchpad.net/libdbusmenu-qt"
                DESCRIPTION "a library implementing the DBusMenu specification"
                PURPOSE     "Required for having a context menu for the D-Bus-based tray icon"
            )
        endif()

        if (WITH_WEBKIT AND QT_QTWEBKIT_FOUND)
            set(HAVE_WEBKIT true)
        endif()
        add_feature_info("WITH_WEBKIT and QtWebKit module" HAVE_WEBKIT "Support showing previews for URLs in chat")

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

        else()
            find_package(Phonon QUIET)
            set_package_properties(Phonon PROPERTIES TYPE RECOMMENDED
                URL "https://projects.kde.org/projects/kdesupport/phonon"
                DESCRIPTION "a multimedia abstraction library"
                PURPOSE     "Required for audio notifications"
            )
        endif()

        find_package(IndicateQt QUIET)
        set_package_properties(IndicateQt PROPERTIES TYPE OPTIONAL
            URL "https://launchpad.net/libindicate-qt/"
            DESCRIPTION "a library to raise flags on DBus for other components of the desktop to pick up and visualize"
            PURPOSE     "Provides integration into the Ayatana notification system used by e.g. Ubuntu"
        )

    endif()

    if (BUILD_CORE)

        find_package(QCA2 QUIET)
        set_package_properties(QCA2 PROPERTIES TYPE RECOMMENDED
            URL "https://projects.kde.org/projects/kdesupport/qca"
            DESCRIPTION "Qt Cryptographic Architecture"
            PURPOSE     "Required for encryption support"
        )


    endif()

    # Qt4 does not consider lconvert relevant, so they don't support finding it...
    # Rather than shipping hacked buildsys files, let's just infer the path from lrelease
    if (QT_LRELEASE_EXECUTABLE)
        get_filename_component(_lrelease_path ${QT_LRELEASE_EXECUTABLE} PATH)
        find_program(QT_LCONVERT_EXECUTABLE NAMES lconvert-qt4 lconvert PATHS ${_lrelease_path} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
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

# Check for SSL support in Qt
# As there's no easy way to get Qt's configuration in particular for Qt5, let's just compile
# a small test program checking the defines. This works for both Qt4 and Qt5.
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES} ${Qt5Core_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

if (USE_QT5 AND Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_REQUIRED_FLAGS "-fPIC -DQT_NO_VERSION_TAGGING")
endif()

check_cxx_source_compiles("
    #include \"qglobal.h\"
    #if defined QT_NO_SSL
    #  error \"No SSL support\"
    #endif
    int main() {}"
    HAVE_SSL)
cmake_pop_check_state()

# Additional compile settings
#####################################################################

# This sets -fPIC and friends if required by the installed Qt5 library
if (USE_QT5 AND Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Needed to compile with mingw without kde
if (MINGW AND NOT KDE4_FOUND)
    add_definitions(-D_WIN32_WINNT=0x0500)
    message(STATUS "Added _WIN32_WINNT=0x0500 definition for MinGW")
    # workaround for bug in mingw gcc 4.0
    add_definitions(-U__STRICT_ANSI__)
endif()

# Sanitize compiler flags - old versions of KDE set -ansi, which breaks -std=c++11
if (CMAKE_COMPILER_IS_GNUCXX)
    string(REPLACE "-ansi" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()

# Setup LDAP Authentication support.
#####################################################################
if (WITH_LDAP)
    find_package(Ldap)
    if (LDAP_FOUND)
        message(STATUS "Enabling LDAP authentication support")
        set(HAVE_LDAP true)
        add_definitions(-DHAVE_LDAP)
    else()
        message(STATUS "Disabling LDAP authentication support")
    endif()
else()
    message(STATUS "Not enabling LDAP authentication support")
endif()

# Setup KDE / KDE Frameworks
#####################################################################

# We want to do this up here, so we have the necessary variables and defines set before
# compiling anything

if (KDE4_FOUND)
    # We always use external icons for KDE4 support, since we use its iconloader rather than Qt's
    set(EMBED_DATA OFF)

    # Better have the compile flags global, even for the core, to avoid problems with linking the mono client
    add_definitions(-DHAVE_KDE -DHAVE_KDE4 ${KDE4_DEFINITIONS})
    set(WITH_KDE4 TRUE)
endif()

if (USE_QT5 AND WITH_KDE)
    # If KDE Frameworks are present, they're most probably providing Qt5 integration including icon loading
    set(EMBED_DATA OFF)

    include(KDEInstallDirs)
    include(KDECompilerSettings)
    include(KDECMakeSettings)

    add_definitions(-DHAVE_KDE -DHAVE_KF5)
    set(WITH_KF5 TRUE)
endif()

# This needs to come after setting up KDE integration, so we can use KDE-specific paths
include(QuasselInstallDirs)

# Various config-dependent checks and settings
#####################################################################

if (NOT ZLIB_FOUND)
    message(STATUS "zlib NOT found, using bundled miniz for compression")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        message(STATUS "WARNING: This may be slow on 32 bit systems!")
    endif()
endif()

if (HAVE_SSL)
    add_definitions(-DHAVE_SSL)
endif()
add_feature_info("SSL support in Qt" HAVE_SSL "Use secure network connections")

# Check for syslog support
if (NOT WIN32)
    check_include_file(syslog.h HAVE_SYSLOG)
    add_feature_info("syslog.h" HAVE_SYSLOG "Provide support for logging to the syslog")
endif()

add_feature_info("Qt Linguist Tools" QT_LCONVERT_EXECUTABLE "Translation support for Quassel")

if (EMBED_DATA)
    message(STATUS "Embedding data files into the binary")
else()
    message(STATUS "Installing data files separately")
endif()

if (INDICATEQT_FOUND)
    add_definitions(-DXDG_APPS_INSTALL_DIR=${CMAKE_INSTALL_APPDIR})
endif()

if (NOT WIN32)
    check_function_exists(umask HAVE_UMASK)
    if(HAVE_UMASK)
        add_definitions(-DHAVE_UMASK)
    endif()
endif()


# Windows-specific stuff
#####################################################################

if (WIN32)
    link_libraries(imm32 winmm dbghelp Secur32)  # missing by default :/
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX")
        set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBUGINFO "/debug /INCREMENTAL:YES /NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:YES /NODEFAULTLIB:libcmt")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUGFULL "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
        link_libraries(Version dwmapi shlwapi)
        if (USE_QT5)
            set(QT_QTMAIN_LIBRARY Qt5::WinMain)
        endif()
    endif()
    if(HAVE_SSL AND STATIC)
        find_package(OpenSSL REQUIRED)
        link_libraries(${OPENSSL_LIBRARIES} ${OPENSSL_EAY_LIBRARIES})
    endif()
endif()


# Static builds (very much non-portable, so don't use -DSTATIC
# unless you know what you do!)
#####################################################################

if(STATIC AND CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-static-libgcc ${CMAKE_CXX_FLAGS}")
    link_directories(${CMAKE_BINARY_DIR}/staticlibs) # override dynamic libs
    if (HAVE_SSL)
        set(QUASSEL_SSL_LIBRARIES ssl crypto)  # these miss in static builds
    endif()
endif()


# Generate version information from Git
#####################################################################

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_HEAD)
git_describe(GIT_DESCRIBE --long)

# If in a Git repo we can get the commit-date from a git command
if (GIT_HEAD)
    execute_process(
        COMMAND git -c log.showsignature=false show -s --format=%ct
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

# If not in a Git repo try to read GIT_HEAD and GIT_DESCRIBE from
# enviroment
if (NOT GIT_HEAD OR NOT GIT_DESCRIBE)
  if (DEFINED ENV{GIT_HEAD})
      set(GIT_HEAD $ENV{GIT_HEAD})
  endif()
  if (DEFINED ENV{GIT_DESCRIBE})
     set(GIT_DESCRIBE $ENV{GIT_DESCRIBE})
  endif()
endif()

# Sanitize things if we're not in a Git repo
if (NOT GIT_HEAD OR NOT GIT_DESCRIBE)
    set(GIT_HEAD "")
    set(GIT_DESCRIBE "")
    set(GIT_COMMIT_DATE 0)
endif()

configure_file(version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)

# Prepare the build
#####################################################################

# These variables will be added to the main targets (CORE, QTCLIENT, MONO)
set(COMMON_DEPS ${RC_WIN32})
set(CORE_DEPS )
set(CLIENT_DEPS )

# Add needed subdirs - the order is important, since src needs some vars set by other dirs
add_subdirectory(data)
add_subdirectory(icons)
add_subdirectory(pics)
add_subdirectory(po)


# Set up and display feature summary
#####################################################################

feature_summary(WHAT ALL
                INCLUDE_QUIET_PACKAGES
                FATAL_ON_MISSING_REQUIRED_PACKAGES
)

# Finally, compile the sources
# We want this after displaying the feature summary to avoid ugly
# CMake backtraces in case a required Qt5 module is missing
#####################################################################

add_subdirectory(src)

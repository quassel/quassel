# Main CMake file for building Quassel IRC
#
# See INSTALL for possible CMake options (or read the code, Luke)
#####################################################################

# General setup
#####################################################################

cmake_minimum_required(VERSION 3.5)

# Tell CMake about or own modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Versions
set(QUASSEL_MAJOR  0)
set(QUASSEL_MINOR 13)
set(QUASSEL_PATCH 50)
set(QUASSEL_VERSION_STRING "0.14-pre")

# Output CMake and Quassel versions as well as build type for debug reasons
message(STATUS "Building Quassel ${QUASSEL_VERSION_STRING}...")
message(STATUS "Using CMake ${CMAKE_VERSION}")

# Set up build type rather early
include(BuildType)

# Support ccache if found
# This should happen before calling project(), so compiler settings are validated.
option(USE_CCACHE "Enable support for ccache if available" ON)
if (USE_CCACHE)
    message(STATUS "Checking for ccache")
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
        message(STATUS "Checking for ccache - enabled")
    else()
        message(STATUS "Checking for ccache - not found")
    endif()
endif()

# Set up project
project(Quassel CXX)

# Let CMake handle file generation for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Needed, otherwise some .moc files won't be found with older CMake versions
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include various CMake modules...
include(CMakePushCheckState)
include(CheckFunctionExists)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)
include(CMakeDependentOption)
include(FeatureSummary)

# ... and our own
include(QuasselCompileSettings)
include(QuasselMacros)

# Options and variables that can be set on the command line
#####################################################################

# Select the binaries to build
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the client-only binary"             ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" ON)
add_feature_info(WANT_CORE WANT_CORE "Build the core (server) binary")
add_feature_info(WANT_QTCLIENT WANT_QTCLIENT "Build the client-only binary (requires a core to connect to)")
add_feature_info(WANT_MONO WANT_MONO "Build the monolithic (all-in-one) binary")

# Whether to enable integration with higher-tier KDE frameworks that require runtime support.
# We still optionally make use of certain Tier 1 frameworks even if WITH_KDE is disabled.
option(WITH_KDE "Integration with the KDE Frameworks runtime environment")
add_feature_info(WITH_KDE WITH_KDE "Integrate with the KDE Frameworks runtime environment")

# Icon theme support. By default, install the Breeze icon theme (may be disabled if a system installation is present)
option(WITH_BUNDLED_ICONS "Install required icons from the Breeze icon theme" ON)
add_feature_info(WITH_BUNDLED_ICONS WITH_BUNDLED_ICONS "Install required icons from the Breeze icon theme")

option(WITH_OXYGEN_ICONS "Support the Oxygen icon theme (KDE4)" OFF)
add_feature_info(WITH_OXYGEN_ICONS WITH_OXYGEN_ICONS "Support the Oxygen icon theme (KDE4)")

if (WITH_BUNDLED_ICONS)
    add_definitions(-DWITH_BUNDLED_ICONS)
endif()
if (WITH_OXYGEN_ICONS)
    add_definitions(-DWITH_OXYGEN_ICONS)
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

# Always embed on Windows or OSX; never embed when enabling KDE integration
set(EMBED_DEFAULT OFF)
if (WIN32 OR APPLE)
    set(EMBED_DEFAULT ON)
endif()
cmake_dependent_option(EMBED_DATA "Embed icons and translations into the binaries instead of installing them" ${EMBED_DEFAULT}
                                   "NOT WIN32;NOT WITH_KDE" ${EMBED_DEFAULT})
if (NOT EMBED_DEFAULT)
    add_feature_info(EMBED_DATA EMBED_DATA "Embed icons and translations in the binaries instead of installing them")
endif()

# The following options are not for end-user consumption, so don't list them in the feature summary
cmake_dependent_option(DEPLOY "Add required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF "APPLE" OFF)


# List of authenticators and the cmake flags to build them
# (currently that's just LDAP, but more can be added here).
####################################################################
option(WITH_LDAP "Enable LDAP authentication support if present on system" ON)

# Setup CMake
#####################################################################

# Let automoc/autouic process generated files
if (POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
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

set(QT_MIN_VERSION "5.5.0")
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
            find_package(KF5 COMPONENTS ConfigWidgets CoreAddons Notifications NotifyConfig Sonnet TextWidgets WidgetsAddons XmlGui QUIET)
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

    find_package(Qca-qt5 2.0)
    set_package_properties(Qca-qt5 PROPERTIES TYPE RECOMMENDED
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

# Non-Qt-based packages

find_package(ZLIB REQUIRED)
set_package_properties(ZLIB PROPERTIES TYPE REQUIRED
    URL "http://www.zlib.net"
    DESCRIPTION "a popular compression library"
    PURPOSE     "Used for protocol compression"
)

if (NOT WIN32)
    # Needed for generating backtraces
    find_package(Backtrace QUIET)
    set_package_properties(Backtrace PROPERTIES TYPE RECOMMENDED
        DESCRIPTION "a header (and possibly library) for inspecting backtraces"
        PURPOSE "Used for generating backtraces in case of a crash"
    )
endif()

# Check for SSL support in Qt
# As there's no easy way to get Qt's configuration in particular for Qt5, let's just compile
# a small test program checking the defines. This works for both Qt4 and Qt5.
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES} ${Qt5Core_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

if (Qt5_POSITION_INDEPENDENT_CODE)
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
if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_REQUIRED_FLAGS "-DQT_NO_VERSION_TAGGING")
endif()

# Needed to compile with mingw without kde
if (MINGW AND NOT WITH_KDE)
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
    else()
        message(STATUS "Disabling LDAP authentication support")
    endif()
else()
    message(STATUS "Not enabling LDAP authentication support")
endif()

# Setup support for KDE Frameworks
#####################################################################

# We want to do this up here, so we have the necessary variables and defines set before
# compiling anything

if (WITH_KDE)
    # If KDE Frameworks are present, they're most probably providing Qt5 integration including icon loading
    set(EMBED_DATA OFF)

    include(KDEInstallDirs)
    include(KDECompilerSettings)
    include(KDECMakeSettings)

    kde_enable_exceptions()
    add_definitions(-DHAVE_KDE -DHAVE_KF5)
    set(WITH_KF5 TRUE)
endif()

# This needs to come after setting up KDE integration, so we can use KDE-specific paths
include(QuasselInstallDirs)

# Various config-dependent checks and settings
#####################################################################

if (HAVE_SSL)
    add_definitions(-DHAVE_SSL)
endif()
add_feature_info("SSL support in Qt" HAVE_SSL "Use secure network connections")

# Check for syslog support
if (NOT WIN32)
    check_include_file_cxx(syslog.h HAVE_SYSLOG)
    add_feature_info("syslog.h" HAVE_SYSLOG "Provide support for logging to the syslog")
endif()

add_feature_info("Qt Linguist Tools" QT_LCONVERT_EXECUTABLE "Translation support for Quassel")

if (EMBED_DATA)
    message(STATUS "Embedding data files into the binary")
else()
    message(STATUS "Installing data files separately")
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
        set(QT_QTMAIN_LIBRARY Qt5::WinMain)
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

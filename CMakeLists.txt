# Main CMake file for building Quassel IRC
#
# See INSTALL for possible CMake options (or read the code, Luke)
#####################################################################

# General setup
#####################################################################

cmake_minimum_required(VERSION 3.16)

# Tell CMake about or own modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(QuasselVersion)

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

# Global cache variables for Qt and KF to ease maintenance
set(QT_PREFIX "Qt" CACHE STRING "Qt prefix")
set(QT_VERSION "6" CACHE STRING "Qt version")
set(QT_FULL "${QT_PREFIX}${QT_VERSION}" CACHE STRING "Qt full version")

set(KF_PREFIX "KF" CACHE STRING "KF prefix")
set(KF_VERSION "6" CACHE STRING "KF version")
set(KF_FULL "${KF_PREFIX}${KF_VERSION}" CACHE STRING "KF full version")

# Global cache variables for MacOS
set(MACOS_VERSION "14.0" CACHE STRING "MacOS Version")

# Set C++17 standard for Qt6
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# For this, the feature info is added after we know if QtWebEngine is installed
option(WITH_WEBENGINE "WebEngine support (for link previews)" ON)

# TODO: Further testing on MacOS after Qt6 port
if (APPLE)
    # Notification Center is only available in > 10.8, which is Darwin v12
    if (NOT CMAKE_SYSTEM_VERSION VERSION_LESS 12)
        option(WITH_NOTIFICATION_CENTER "OS X Notification Center support" ON)
        add_feature_info(WITH_NOTIFICATION_CENTER WITH_NOTIFICATION_CENTER "Use the OS X Notification Center")
    endif()
    find_library(FOUNDATION_FRAMEWORK Foundation)
    mark_as_advanced(FOUNDATION_FRAMEWORK)
    link_libraries(${FOUNDATION_FRAMEWORK})

    # Whether to enable the creation of bundles and DMG images
    cmake_dependent_option(BUNDLE "Create bundles and DMG images" OFF "APPLE" OFF)
    add_feature_info(BUNDLE BUNDLE "Create bundles and DMG images")
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

# The following option is not for end-user consumption, so don't list it in the feature summary
option(FATAL_WARNINGS "Make compile warnings fatal (most useful for CI builds)" OFF)

# List of authenticators and the cmake flags to build them
# (currently that's just LDAP, but more can be added here).
####################################################################
option(WITH_LDAP "Enable LDAP authentication support if present on system" ON)

# Setup CMake
#####################################################################

# Visibility settings apply to all targets
if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

# Let automoc/autouic process generated files
if (POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

set(BUILD_SHARED_LIBS TRUE CACHE BOOL "" FORCE)

# Don't use X11 on OSX
if (APPLE)
    set(CMAKE_DISABLE_FIND_PACKAGE_X11 true)
    set(CMAKE_DISABLE_FIND_PACKAGE_XCB true)
    set(CMAKE_DISABLE_FIND_PACKAGE_Qt6X11Extras true)
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

set(QT_MIN_VERSION "6.8")

# Enable Qt deprecation warnings for Qt < 5.13 (on by default in newer versions)
add_definitions(-DQT_DEPRECATED_WARNINGS)

# Disable all Qt APIs that were deprecated in 5.5 and before
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050500)

# Find package dependencies
#
# Note that you can forcefully disable optional packages
# using -DCMAKE_DISABLE_FIND_PACKAGE_<PkgName>=TRUE
#####################################################################

# Required Qt components
set(qt_components Core Network)
if (BUILD_GUI)
    list(APPEND qt_components Gui Widgets)
endif()
if (BUILD_CORE)
    list(APPEND qt_components Sql)
endif()

find_package(${QT_FULL} ${QT_MIN_VERSION} REQUIRED COMPONENTS ${qt_components})
set_package_properties(${QT_FULL} PROPERTIES TYPE REQUIRED
    URL "https://www.qt.io/"
    DESCRIPTION "the Qt libraries"
)
message(STATUS "Found Qt ${Qt6Core_VERSION}")

set(CMAKE_OSX_DEPLOYMENT_TARGET "${MACOS_VERSION}" CACHE STRING "Minimum macOS version")

# Determine minimum deployment target for macOS supported by Qt
if(APPLE)
    if(NOT QMAKE_MACOSX_DEPLOYMENT_TARGET)
        # qmake cannot be queried directly for QMAKE_MACOSX_DEPLOYMENT_TARGET (it is a mkspec, not a property).
        # Instead, invoke qmake on an empty project file, which causes it to output the relevant keys and their values
        # for subsequent parsing.
        # A file named .qmake.stash is always created, so remove it (and empty.pro) afterwards.
        set(qmakeEmptyProjectFile "${CMAKE_BINARY_DIR}/empty.pro")
        set(qmakeStashFile "${CMAKE_BINARY_DIR}/.qmake.stash")
        file(WRITE ${qmakeEmptyProjectFile} "")
        get_target_property(QMAKE_EXECUTABLE ${QT_FULL}::qmake IMPORTED_LOCATION)
        execute_process(
            COMMAND ${QMAKE_EXECUTABLE} -E ${qmakeEmptyProjectFile}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            OUTPUT_VARIABLE qmakeOutput
        )
        file(REMOVE ${qmakeEmptyProjectFile} ${qmakeStashFile})
        string(REGEX MATCH "QMAKE_MACOSX_DEPLOYMENT_TARGET[ ]*=[ ]*([0-9.]+)" foo ${qmakeOutput})
        if(NOT CMAKE_MATCH_1)
            message(FATAL_ERROR "Could not determine the deployment target for Qt")
        endif()
        set(QMAKE_MACOSX_DEPLOYMENT_TARGET ${CMAKE_MATCH_1} CACHE INTERNAL "")
        mark_as_advanced(QMAKE_MACOSX_DEPLOYMENT_TARGET)
    endif()
    message(STATUS "Minimum macOS version supported by Qt: ${QMAKE_MACOSX_DEPLOYMENT_TARGET}")
endif()

# Check for SSL support in Qt
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LIBRARIES ${QT_FULL}::Core ${QT_FULL}::Network)
set(CMAKE_REQUIRED_FLAGS -std=c++14)
check_cxx_source_compiles("
    #include <QSslSocket>
    int main() {
        QSslSocket sock;
        return 0;
    }"
    HAVE_SSL)
cmake_pop_check_state()

if (NOT HAVE_SSL)
    message(FATAL_ERROR "Quassel requires SSL support, but Qt is built with QT_NO_SSL")
endif()

# Optional Qt components
find_package(${QT_FULL}LinguistTools QUIET)
set_package_properties(${QT_FULL}LinguistTools PROPERTIES TYPE RECOMMENDED
    DESCRIPTION "contains tools for handling translation files"
    PURPOSE "Required for having translations"
)

if (BUILD_GUI)
    if (NOT WIN32)
        find_package(${QT_FULL}DBus QUIET)
        set_package_properties(${QT_FULL}Bus PROPERTIES TYPE RECOMMENDED
            URL "https://www.qt.io/"
            DESCRIPTION "D-Bus support for Qt"
            PURPOSE     "Needed for supporting D-Bus-based notifications and tray icon, used by most modern desktop environments"
        )
        if (${QT_FULL}DBus_FOUND)
            find_package(dbusmenu-qt${QT_VERSION} QUIET CONFIG)
            set_package_properties(dbusmenu-qt${QT_VERSION} PROPERTIES TYPE RECOMMENDED
                URL "https://launchpad.net/libdbusmenu-qt"
                DESCRIPTION "a library implementing the DBusMenu specification"
                PURPOSE     "Required for having a context menu for the D-Bus-based tray icon"
            )
        endif()
    endif()

    find_package(${QT_FULL}Multimedia QUIET)
    set_package_properties(${QT_FULL}Multimedia PROPERTIES TYPE RECOMMENDED
        URL "https://www.qt.io/"
        DESCRIPTION "Multimedia support for Qt"
        PURPOSE     "Required for audio notifications"
    )

    # snorenotify segfaults on startup on msys2
    # we don't check for just MSYS to support the Ninja generator
    if(NOT (WIN32 AND (NOT $ENV{MSYSTEM} STREQUAL "")))
        find_package(LibsnoreQt5 0.7.0 QUIET)
        set_package_properties(LibsnoreQt5 PROPERTIES TYPE OPTIONAL
            URL "https://projects.kde.org/projects/playground/libs/snorenotify"
            DESCRIPTION "a cross-platform notification framework"
            PURPOSE     "Enable support for the snorenotify framework"
        )
        if (LibsnoreQt5_FOUND)
            find_package(LibsnoreSettingsQt5 QUIET)
            set_package_properties(LibsnoreSettingsQt5 PROPERTIES TYPE OPTIONAL
                URL "https://projects.kde.org/projects/playground/libs/snorenotify"
                DESCRIPTION "a cross-platform notification framework"
                PURPOSE     "Enable support for the snorenotify framework"
            )
        endif()
    endif()

    if (WITH_WEBENGINE)
        find_package(${QT_FULL}WebEngine QUIET)
        set_package_properties(${QT_FULL}WebEngine PROPERTIES TYPE RECOMMENDED
            URL "https://www.qt.io/"
            DESCRIPTION "a WebEngine implementation for Qt"
            PURPOSE     "Needed for displaying previews for URLs in chat"
        )
        if (${QT_FULL}WebEngine_FOUND)
            find_package(${QT_FULL}WebEngineWidgets QUIET)
            set_package_properties(${QT_FULL}WebEngineWidgets PROPERTIES TYPE RECOMMENDED
                URL "https://www.qt.io/"
                DESCRIPTION "widgets for Qt's WebEngine implementation"
                PURPOSE     "Needed for displaying previews for URLs in chat"
            )
        endif()
    endif()

    if (WITH_WEBENGINE AND ${QT_FULL}WebEngineWidgets_FOUND)
        set(HAVE_WEBENGINE true)
    endif()
    add_feature_info("WITH_WEBENGINE, QtWebEngine and QtWebEngineWidgets modules" HAVE_WEBENGINE "Support showing previews for URLs in chat")

    # KDE Frameworks
    ################

    # extra-cmake-modules
    if (WITH_KDE)
        set(ecm_find_type "REQUIRED")
        find_package(ECM NO_MODULE REQUIRED)
    else()
        # Even with KDE integration disabled, we optionally use tier1 frameworks if we find them
        set(ecm_find_type "RECOMMENDED")
        find_package(ECM NO_MODULE QUIET)
    endif()

    set_package_properties(ECM PROPERTIES TYPE ${ecm_find_type}
        URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules"
        DESCRIPTION "extra modules for CMake, maintained by the KDE project"
        PURPOSE     "Required to find KDE Frameworks components"
    )

    if (ECM_FOUND)
        list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
        if (WITH_KDE)
            find_package(${KF_FULL} REQUIRED COMPONENTS ConfigWidgets CoreAddons Notifications NotifyConfig Sonnet TextWidgets WidgetsAddons XmlGui)
            set_package_properties(${KF_FULL} PROPERTIES TYPE REQUIRED
                URL "http://www.kde.org"
                DESCRIPTION "KDE Frameworks"
                PURPOSE     "Required for integration into the Plasma desktop"
            )
            message(STATUS "Found KDE Frameworks ${KF_VERSION}")
        endif()

        # Optional KF tier1 components
        find_package(${KF_FULL}Sonnet QUIET)
        set_package_properties(${KF_FULL}Sonnet PROPERTIES TYPE RECOMMENDED
            URL "http://api.kde.org/frameworks-api/frameworks5-apidocs/sonnet/html"
            DESCRIPTION "framework for providing spell-checking capabilities"
            PURPOSE "Enables spell-checking support in input widgets"
        )
    endif()
endif()

if (BUILD_CORE)
    find_package(Qca-qt6 2.0 QUIET)
    set_package_properties(Qca-qt5 PROPERTIES TYPE RECOMMENDED
        URL "https://projects.kde.org/projects/kdesupport/qca"
        DESCRIPTION "Qt Cryptographic Architecture"
        PURPOSE "Required for encryption support"
    )

    if (WITH_LDAP)
        find_package(Ldap QUIET)
        set_package_properties(Ldap PROPERTIES TYPE OPTIONAL
            URL "http://www.openldap.org/"
            DESCRIPTION "LDAP (Lightweight Directory Access Protocol) libraries"
            PURPOSE "Enables core user authentication via LDAP"
        )
    endif()
endif()

# Non-Qt-based packages
#####################################################################

find_package(Boost 1.74 REQUIRED)
set_package_properties(Boost PROPERTIES TYPE REQUIRED
    URL "https://www.boost.org/"
    DESCRIPTION "Boost libraries for C++"
)
# Older versions don't define the imported target
if (NOT TARGET Boost::boost)
    add_library(Boost::boost INTERFACE IMPORTED GLOBAL)
    if (Boost_INCLUDE_DIRS)
        set_target_properties(Boost::boost PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
    endif()
endif()

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

# Shared library support
#####################################################################

option(ENABLE_SHARED "Build modules as shared libraries" ON)
add_feature_info(ENABLE_SHARED ENABLE_SHARED "Build modules as shared libraries")

# Setup unit testing
#####################################################################

option(BUILD_TESTING "Enable unit tests" OFF)
add_feature_info(BUILD_TESTING BUILD_TESTING "Build unit tests")

if (BUILD_TESTING)
    find_package(GTest QUIET)
    set_package_properties(GTest PROPERTIES TYPE REQUIRED
        DESCRIPTION "Google's unit testing framework"
        PURPOSE "Required for building unit tests"
    )

    find_package(${QT_FULL}Test QUIET)
    set_package_properties(${QT_FULL}Test PROPERTIES TYPE REQUIRED
        DESCRIPTION "unit testing library for the Qt framework"
        PURPOSE "Required for building unit tests"
    )
    enable_testing()

    # GTest messes with CMAKE_CXX_FLAGS, so process them again
    process_cmake_cxx_flags()
endif()

# Setup support for KDE Frameworks
#####################################################################

if (WITH_KDE)
    add_definitions(-DHAVE_KDE -DHAVE_${KF_FULL})
    set(WITH_${KF_FULL} TRUE)

    # If KDE Frameworks are present, they're most probably providing Qt integration including icon loading
    set(EMBED_DATA OFF)

    include(KDEInstallDirs)
endif()

# This needs to come after setting up KDE integration, so we can use KDE-specific paths
include(QuasselInstallDirs)

# RPATH and output settings
#####################################################################

# Build artifacts in a well-known location; especially important for Windows DLLs
# (which go into RUNTIME_OUTPUT_DIRECTORY and can thus be found by executables)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# These RPATH settings allow for running directly from the build dir
set(CMAKE_SKIP_BUILD_RPATH            FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH    FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

# Set install RPATH only if libdir isn't a system directory
if (IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(libdir "${CMAKE_INSTALL_LIBDIR}")
else()
    set(libdir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif()
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${libdir}" is_systemdir)
if ("${is_systemdir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${libdir}")
endif()

# Various config-dependent checks and settings
#####################################################################

# Check for syslog support
if (NOT WIN32)
    check_include_file_cxx(syslog.h HAVE_SYSLOG)
    add_feature_info("syslog.h" HAVE_SYSLOG "Provide support for logging to the syslog")
endif()

if (NOT WIN32)
    check_function_exists(umask HAVE_UMASK)
endif()

if (EMBED_DATA)
    message(STATUS "Embedding data files into the binary")
else()
    message(STATUS "Installing data files separately")
endif()

# Windows-specific stuff
#####################################################################

if (WIN32)
    link_libraries(imm32 winmm dbghelp Secur32)  # missing by default :/
    if (MSVC)
        link_libraries(Version dwmapi shlwapi)
    endif()
endif()

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
# CMake backtraces in case a required Qt module is missing
#####################################################################

add_subdirectory(src)

# Build tests if so desired
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

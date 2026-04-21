# Main CMake file for building Quassel IRC
#
# See INSTALL for possible CMake options (or read the code, Luke)
#####################################################################

# General setup
#####################################################################

cmake_minimum_required(VERSION 3.20)

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

if (APPLE)
    # Notification Center is only available in > 10.8, which is Darwin v12
    if (NOT CMAKE_SYSTEM_VERSION VERSION_LESS 12)
        option(WITH_NOTIFICATION_CENTER "OS X Notification Center support" ON)
        add_feature_info(WITH_NOTIFICATION_CENTER WITH_NOTIFICATION_CENTER "Use the OS X Notification Center")
    endif()
    find_library(CARBON_LIBRARY Carbon)
    mark_as_advanced(CARBON_LIBRARY)
    link_libraries(${CARBON_LIBRARY})

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

# Keep Qt deprecation warnings enabled during the migration
add_definitions(-DQT_DEPRECATED_WARNINGS)

# Keep the disabled-deprecations floor low until the source port is further along
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050500)

# Find package dependencies
#
# Note that you can forcefully disable optional packages
# using -DCMAKE_DISABLE_FIND_PACKAGE_<PkgName>=TRUE
#####################################################################

include(QuasselQtSetup)

# Non-Qt-based packages
#####################################################################

find_package(Boost 1.70 CONFIG REQUIRED)
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

    find_package(Qt6 ${QUASSEL_QT_MIN_VERSION} QUIET COMPONENTS Test)
    set_package_properties(Qt6Test PROPERTIES TYPE REQUIRED
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
    add_definitions(-DHAVE_KDE -DHAVE_KF6)
    set(WITH_KF TRUE)

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
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/po/CMakeLists.txt")
    add_subdirectory(po)
else()
    message(WARNING "Skipping translations because the 'po' submodule is not checked out")
endif()

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

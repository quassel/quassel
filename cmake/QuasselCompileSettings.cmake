# This file contains compile flags and general build configuration for Quassel
#
# (C) 2014 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXCompilerFlag)

if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo Debug Debugfull Profile)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "These are the configuration types we support" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo Debug Debugfull Profile" FORCE)
endif()

# Qt debug flags
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG QT_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUGFULL QT_DEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE QT_NO_DEBUG NDEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO QT_NO_DEBUG NDEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_PROFILE QT_NO_DEBUG NDEBUG)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_MINSIZEREL QT_NO_DEBUG NDEBUG)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS QT_NO_DEBUG NDEBUG)
endif()

# Enable various flags on gcc
if (CMAKE_COMPILER_IS_GNUCXX)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
        message(FATAL_ERROR "Your compiler is too old; you need GCC 4.8+, Clang 3.3+, MSVC 19.0+, or any other compiler with full C++11 support.")
    endif()

    # Let's just hope that all gccs support these options and skip the tests...
    # -fno-strict-aliasing is needed apparently for Qt < 4.6
    set(CMAKE_CXX_FLAGS                  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wnon-virtual-dtor -fno-strict-aliasing -Wundef -Wcast-align -Wpointer-arith -Wformat-security -fno-check-new -fno-common")
    #  set(CMAKE_CXX_FLAGS_RELEASE          "-O2")   # use CMake default
    #  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")  # use CMake default
    set(CMAKE_CXX_FLAGS_DEBUG             "-g -ggdb -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
    set(CMAKE_CXX_FLAGS_DEBUGFULL         "-g3 -ggdb -fno-inline")
    set(CMAKE_CXX_FLAGS_PROFILE           "-g3 -ggdb -fno-inline -ftest-coverage -fprofile-arcs")

    check_cxx_compiler_flag(-Woverloaded-virtual CXX_W_OVERLOADED_VIRTUAL)
    if(CXX_W_OVERLOADED_VIRTUAL)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
    endif()

    # Just for miniz
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-function -Wno-undef -fno-strict-aliasing")

# ... and for Clang
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.3")
        message(FATAL_ERROR "Your compiler is too old; you need Clang 3.3+, GCC 4.8+, MSVC 19.0+, or any other compiler with full C++11 support.")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wnon-virtual-dtor -Wno-long-long -Wundef -Wcast-align -Wchar-subscripts -Wall -W -Wextra -Wpointer-arith -Wformat-security -Woverloaded-virtual -fno-common -Wno-deprecated-register")
    #  set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG -DQT_NO_DEBUG")     # Use CMake default
    #  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -DQT_NO_DEBUG")  # Use CMake default
    set(CMAKE_CXX_FLAGS_DEBUG          "-g -O2 -fno-inline")
    set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline")
    set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -fno-inline -ftest-coverage -fprofile-arcs")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-function -Wno-undef -fno-strict-aliasing")

# For MSVC, at least do a version sanity check
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.0")
        message(FATAL_ERROR "Your compiler is too old; you need at least Visual Studio 2015 (MSVC 19.0+), GCC 4.8+, Clang 3.3+, or any other compiler with full C++11 support.")
    endif()

# Unknown/unsupported compiler
else()
    message(WARNING "Unknown or unsupported compiler. Make sure to enable C++11 support. Good luck.")
endif()

# Mac build stuff
if (APPLE AND DEPLOY)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.9 -stdlib=libc++")
endif()

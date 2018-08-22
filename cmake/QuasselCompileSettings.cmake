# This file contains compile flags and general build configuration for Quassel
#
# (C) 2014-2018 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Helper function to check for linker flag support
include(CheckCXXCompilerFlag)
function(check_and_set_linker_flag flag name outvar)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "${flag}")
    check_cxx_compiler_flag("" LINKER_SUPPORTS_${name})
    if (LINKER_SUPPORTS_${name})
        set(${outvar} "${${outvar}} ${flag}" PARENT_SCOPE)
    endif()
    cmake_pop_check_state()
endfunction()

# General compile settings
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)    # Rely on compile features if standard is not supported
set(CMAKE_CXX_EXTENSIONS OFF)           # We like to be standard conform
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# For GCC and Clang, enable a whole bunch of warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wcast-align
        -Wextra
        -Wformat-security
        -Wno-unknown-pragmas
        -Wnon-virtual-dtor
        -Wpedantic
        -Wundef
        -fno-common
        -fstack-protector-strong
        -fvisibility=default
        -fvisibility-inlines-hidden
        "$<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE;-D_FORTIFY_SOURCE=2>"
    )

    # Check for and set linker flags
    check_and_set_linker_flag("-Wl,-z,relro"    RELRO     LINKER_FLAGS)
    check_and_set_linker_flag("-Wl,-z,now"      NOW       LINKER_FLAGS)
    check_and_set_linker_flag("-Wl,--as-needed" AS_NEEDED LINKER_FLAGS)

    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
else()
    # For other compilers, we rely on default settings (unless someone provides a good set of options; patches welcome!)
endif()

# Mac build stuff
if (APPLE AND DEPLOY)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    add_compile_options(
        -mmacosx-version-min=10.9
        -stdlib=libc++
    )
endif()

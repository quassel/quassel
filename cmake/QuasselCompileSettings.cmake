# This file contains compile flags and general build configuration for Quassel
#
# (C) 2014-2022 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(QuasselMacros)

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

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# For GCC and Clang, enable a whole bunch of warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -fdiagnostics-color=always
        -fexceptions
        -fno-common
        -Wall
        -Wextra
        -Wcast-align
        -Wformat-security
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wpedantic
        -Wundef
        -Wvla
        -Werror=return-type
        "$<$<BOOL:${FATAL_WARNINGS}>:-Werror>"
        -Wno-error=deprecated-declarations  # Don't break on Qt upgrades
        -Wno-unknown-pragmas
        "$<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE;-D_FORTIFY_SOURCE=2>"
    )

    # ssp is currently very broken on MinGW
    if(NOT MINGW)
        add_compile_options(-fstack-protector-strong)
    endif()

    # Check for and set linker flags
    check_and_set_linker_flag("-Wl,-z,relro"            RELRO            LINKER_FLAGS)
    check_and_set_linker_flag("-Wl,-z,now"              NOW              LINKER_FLAGS)
    check_and_set_linker_flag("-Wl,--as-needed"         AS_NEEDED        LINKER_FLAGS)
    check_and_set_linker_flag("-Wl,--enable-new-dtags"  ENABLE_NEW_DTAGS LINKER_FLAGS)
    check_and_set_linker_flag("-Wl,--no-undefined"      NO_UNDEFINED     LINKER_FLAGS)

    set(CMAKE_EXE_LINKER_FLAGS    "${LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS "${LINKER_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")

elseif(MSVC)
    # Target Windows Vista
    add_definitions(-D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -D_WIN32_IE=0x0600)

    # Various settings for the Windows API
    add_definitions(-DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE -D_USE_MATH_DEFINES -DNOMINMAX)

    # Compile options
    add_compile_options(
        /EHsc
        "$<$<BOOL:${FATAL_WARNINGS}>:/WX>"
    )

    # Increase warning level on MSVC
    # CMake puts /W3 in CMAKE_CXX_FLAGS which will be appended later, so we need to replace
    string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    # Silence warnings that are hard or impossible to avoid
    #   C4127: conditional expression is constant [in qvector.h as of Qt 5.15.2]
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127")
    #   C4456: declaration of 'identifier' hides previous local declaration [caused by foreach macro, among others]
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4456")
    #   C4458: declaration of 'identifier' hides class member [caused by MOC for Peer::_id]
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458")
    #   C4996: deprecation warnings [unavoidable due to wide range of supported Qt versions]
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
    #   C5240: 'nodiscard': attribute is ignored in this syntactic position [in qcolor.h as of Qt 5.15.2]
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd5240")

    # Link against the correct version of the C runtime
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "/NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt ${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/NODEFAULTLIB:libcmtd /DEFAULTLIB:msvcrtd ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

else()
    # For other compilers, we rely on default settings (unless someone provides a good set of options; patches welcome!)
    message(WARNING "${CMAKE_CXX_COMPILER_ID} is not a supported C++ compiler.")
endif()

# Mac build stuff
if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    add_compile_options(
        -mmacosx-version-min=10.9
        -stdlib=libc++
    )
    add_definitions(-DQT_MAC_USE_COCOA -D_DARWIN_C_SOURCE)
endif()

# Optionally, produce clazy warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_CLAZY "Enable Clazy warnings" OFF)

    if(ENABLE_CLAZY)
        set(CMAKE_CXX_COMPILE_OBJECT "${CMAKE_CXX_COMPILE_OBJECT} -Xclang -load -Xclang ClangLazy${CMAKE_SHARED_LIBRARY_SUFFIX} -Xclang -add-plugin -Xclang clang-lazy")
    endif()
endif()

# Append CMAKE_CXX_FLAGS, so our flags can be overwritten externally.
process_cmake_cxx_flags()

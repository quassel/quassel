# This file contains various functions and macros useful for building Quassel.
#
# (C) 2014-2018 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
###################################################################################################

include(CMakeParseArguments)
include(GenerateExportHeader)
include(QuasselCompileFeatures)

###################################################################################################
# Adds a library target for a Quassel module.
#
# quassel_add_module(Module [STATIC] [EXPORT])
#
# The function expects the (CamelCased) module name as a parameter, and derives various
# strings from it. For example, quassel_add_module(Client) produces
#  - a library target named quassel_client with output name (lib)quassel-client(.so)
#  - an alias target named Quassel::Client in global scope
#
# If the optional argument STATIC is given, a static library is built; otherwise, on
# platforms other than Windows, a shared library is created. For shared libraries, also
# an install rule is added.
#
# To generate an export header for the library, specify EXPORT. The header will be named
# ${module}-export.h (where ${module} is the lower-case name of the module).
#
# The function exports the TARGET variable which can be used in the current scope
# for setting source files, properties, link dependencies and so on.
# To refer to the target outside of the current scope, e.g. for linking, use
# the alias name.
#
function(quassel_add_module _module)
    set(options EXPORT STATIC NOINSTALL)
    set(oneValueArgs )
    set(multiValueArgs )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Derive target, alias target, output name from the given module name
    set(alias "Quassel::${_module}")
    set(target ${alias})
    string(TOLOWER ${target} target)
    string(REPLACE "::" "_" target ${target})
    string(REPLACE "_" "-" output_name ${target})

    if (ARG_STATIC)
        set(buildmode STATIC)
    else()
        set(buildmode SHARED)
    endif()

    add_library(${target} ${buildmode} "")
    add_library(${alias} ALIAS ${target})

    target_link_libraries(${target} PRIVATE Qt5::Core)
    target_include_directories(${target}
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR} # for generated files
    )
    target_compile_features(${target} PUBLIC ${QUASSEL_COMPILE_FEATURES})

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${output_name}
        VERSION ${QUASSEL_MAJOR}.${QUASSEL_MINOR}.${QUASSEL_PATCH}
    )

    if (buildmode STREQUAL "SHARED" AND NOT ${ARG_NOINSTALL})
        install(TARGETS ${target}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/quassel
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/quassel
        )
    endif()

    if (ARG_EXPORT)
        string(TOLOWER ${_module} lower_module)
        string(TOUPPER ${_module} upper_module)
        string(REPLACE "::" "-" header_base ${lower_module})
        string(REPLACE "::" "_" macro_base ${upper_module})
        generate_export_header(${target}
            BASE_NAME ${macro_base}
            EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/export/${header_base}-export.h
        )
        target_include_directories(${target} PUBLIC ${CMAKE_BINARY_DIR}/export)
    endif()

    # Export the target name for further use
    set(TARGET ${target} PARENT_SCOPE)
endfunction()

###################################################################################################
# Provides a library that contains data files as a Qt resource (.qrc).
#
# quassel_add_resource(QrcName
#                      [BASEDIR basedir]
#                      [PREFIX prefix]
#                      PATTERNS pattern1 pattern2...
#                      [DEPENDS dep1 dep2...]
# )
#
# The first parameter is the CamelCased name of the resource; the library target will be called
# "Quassel::Resource::QrcName". The library provides a Qt resource named "qrcname" (lowercased QrcName)
# containing the files matching PATTERNS relative to BASEDIR (by default, the current source dir).
# The resource prefix can be set by giving the PREFIX argument.
# Additional target dependencies can be specified with DEPENDS.
#
function(quassel_add_resource _name)
    set(options )
    set(oneValueArgs BASEDIR PREFIX)
    set(multiValueArgs DEPENDS PATTERNS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_BASEDIR)
        set(ARG_BASEDIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    get_filename_component(basedir ${ARG_BASEDIR} REALPATH)

    string(TOLOWER ${_name} lower_name)

    set(qrc_target   quassel-qrc-${lower_name})
    set(qrc_file     ${lower_name}.qrc)
    set(qrc_src      qrc_${lower_name}.cpp)
    set(qrc_filepath ${CMAKE_CURRENT_BINARY_DIR}/${qrc_file})
    set(qrc_srcpath  ${CMAKE_CURRENT_BINARY_DIR}/${qrc_src})

    # This target will always be built, so the qrc file will always be freshly generated.
    # That way, changes to the glob result are always taken into account.
    add_custom_target(${qrc_target} VERBATIM
        COMMENT "Generating ${qrc_file}"
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/GenerateQrc.cmake "${qrc_filepath}" "${ARG_PREFIX}" "${ARG_PATTERNS}"
        DEPENDS ${ARG_DEPENDS}
        BYPRODUCTS ${qrc_filepath}
        WORKING_DIRECTORY ${basedir}
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${qrc_filepath})

    # RCC sucks and expects the data files relative to the qrc file, with no way to configure it differently.
    # Only when reading from stdin ("-") it takes the working directory as a base, so we have to use this if
    # we want to use generated qrc files (which obviously cannot be placed in the source directory).
    # Since neither autorcc nor qt5_add_resources() support this, we have to invoke rcc manually :(
    #
    # On Windows, input redirection apparently doesn't work, however piping does. Use this for all platforms for
    # consistency, accommodating for the fact that the 'cat' equivalent on Windows is 'type'.
    if (WIN32)
        set(cat_cmd type)
    else()
        set(cat_cmd cat)
    endif()
    add_custom_command(VERBATIM
        COMMENT "Generating ${qrc_src}"
        COMMAND ${cat_cmd} "$<SHELL_PATH:${qrc_filepath}>"
                | "$<SHELL_PATH:$<TARGET_FILE:Qt5::rcc>>" --name "${lower_name}" --output "$<SHELL_PATH:${qrc_srcpath}>" -
        DEPENDS ${qrc_target}
        MAIN_DEPENDENCY ${qrc_filepath}
        OUTPUT ${qrc_srcpath}
        WORKING_DIRECTORY ${basedir}
    )

    # Generate library target that can be referenced elsewhere. Force static, because
    # we can't easily export symbols from the generated sources.
    quassel_add_module(Resource::${_name} STATIC)
    target_sources(${TARGET} PRIVATE ${qrc_srcpath})
    set_target_properties(${TARGET} PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

    # Set variable for referencing the target from outside
    set(RESOURCE_TARGET ${TARGET} PARENT_SCOPE)
endfunction()

###################################################################################################
# Adds a unit test case
#
# quassel_add_test(TestName
#                  [LIBRARIES lib1 lib2...]
# )
#
# The test name is given in CamelCase as first and mandatory parameter. The corresponding source file
# is expected the lower-cased test name plus the .cpp extension.
# The test case is automatically linked against Qt5::Test, GMock, Quassel::Common and
# Quassel::Test::Main, which contains the main function. This main function also instantiates a
# QCoreApplication, so the event loop can be used in test cases.
#
# Additional libraries can be given using the LIBRARIES argument.
#
# Test cases should include testglobal.h, which transitively includes the GTest/GMock headers and
# exports the main function.
#
# The compiled test case binary is located in the unit/ directory in the build directory.
#
function(quassel_add_test _target)
    set(options )
    set(oneValueArgs )
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    string(TOLOWER ${_target} lower_target)
    set(srcfile ${lower_target}.cpp)

    list(APPEND ARG_LIBRARIES
        Qt5::Test
        Quassel::Common
        Quassel::Test::Global
        Quassel::Test::Main
    )

    if (WIN32)
        # On Windows, tests need to be built in the same directory that contains the libraries
        set(output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else()
        # On other platforms, separate the test cases out
        set(output_dir "${CMAKE_BINARY_DIR}/unit")
    endif()

    add_executable(${_target} ${srcfile})
    set_target_properties(${_target} PROPERTIES
        OUTPUT_NAME ${_target}
        RUNTIME_OUTPUT_DIRECTORY "${output_dir}"
    )
    target_link_libraries(${_target} PUBLIC ${ARG_LIBRARIES})

    add_test(
        NAME ${_target}
        COMMAND $<TARGET_FILE:${_target}>
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endfunction()

###################################################################################################
# target_link_if_exists(Target
#                       [PUBLIC dep1 dep2...]
#                       [PRIVATE dep3 dep4...]
# )
#
# Convenience function to add dependencies to a target only if they exist. This is useful when
# handling targets that are conditionally created, e.g. resource libraries depending on -DEMBED_DATA.
#
# NOTE: In order to link a given target, it must already have been created, i.e its subdirectory
#       must already have been added. This is also true for globally visible ALIAS targets that
#       can otherwise be linked to regardless of creation order; "if (TARGET...)" does not
#       support handling this case correctly.
#
function(target_link_if_exists _target)
    set(options )
    set(oneValueArgs )
    set(multiValueArgs PUBLIC PRIVATE)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (ARG_PUBLIC)
        foreach(dep ${ARG_PUBLIC})
            if (TARGET ${dep})
                target_link_libraries(${_target} PUBLIC ${dep})
            endif()
        endforeach()
    endif()

    if (ARG_PRIVATE)
        foreach(dep ${ARG_PRIVATE})
            if (TARGET ${dep})
                target_link_libraries(${_target} PRIVATE ${dep})
            endif()
        endforeach()
    endif()
endfunction()

###################################################################################################
# process_cmake_cxx_flags()
#
# Append the options declared CMAKE_CXX_FLAGS and CMAKE_CXX_FLAGS_<BUILD_TYPE> to the global
# compile options.
# Unset the variables afterwards to avoid duplication.
#
function(process_cmake_cxx_flags)
    string(TOUPPER ${CMAKE_BUILD_TYPE} upper_build_type)
    set(cxx_flags "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${upper_build_type}}")
    separate_arguments(sep_cxx_flags UNIX_COMMAND ${cxx_flags})
    add_compile_options(${sep_cxx_flags})
    set(CMAKE_CXX_FLAGS "" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS_${upper_build_type} "" PARENT_SCOPE)
endfunction()

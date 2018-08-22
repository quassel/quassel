# This file contains various functions and macros useful for building Quassel.
#
# (C) 2014-2018 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
###################################################################################################

include(CMakeParseArguments)
include(QuasselCompileFeatures)

###################################################################################################
# Adds a library target for a Quassel module.
#
# It expects the (CamelCased) module name as a parameter, and derives various
# strings from it. For example, quassel_add_module(Client) produces
#  - a library target named quassel_client with output name (lib)quassel-client(.so)
#  - an alias target named Quassel::Client in global scope
#
# The function exports the TARGET variable which can be used in the current scope
# for setting source files, properties, link dependencies and so on.
# To refer to the target outside of the current scope, e.g. for linking, use
# the alias name.
#
function(quassel_add_module _module)
    # Derive target, alias target, output name from the given module name
    set(alias "Quassel::${_module}")
    set(target ${alias})
    string(TOLOWER ${target} target)
    string(REPLACE "::" "_" target ${target})
    string(REPLACE "_" "-" output_name ${target})

    add_library(${target} STATIC "")
    add_library(${alias} ALIAS ${target})

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${output_name}
    )

    target_include_directories(${target}
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR} # for generated files
    )

    target_compile_features(${target} PUBLIC ${QUASSEL_COMPILE_FEATURES})

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

    # Generate library target that can be referenced elsewhere
    quassel_add_module(Resource::${_name})
    target_sources(${TARGET} PRIVATE ${qrc_srcpath})
    set_target_properties(${TARGET} PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

    # Set variable for referencing the target from outside
    set(RESOURCE_TARGET ${TARGET} PARENT_SCOPE)
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

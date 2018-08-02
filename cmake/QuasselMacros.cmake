# This file contains various functions and macros useful for building Quassel.
#
# (C) 2014-2018 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
###################################################################################################

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

    # Export the target name for further use
    set(TARGET ${target} PARENT_SCOPE)
endfunction()

######################################
# Macros for dealing with translations
######################################

# This generates a .ts from a .po file
macro(generate_ts outvar basename)
  set(input ${basename}.po)
  set(output ${CMAKE_BINARY_DIR}/po/${basename}.ts)
  add_custom_command(OUTPUT ${output}
          COMMAND $<TARGET_PROPERTY:Qt5::lconvert,LOCATION>
          ARGS -i ${input}
               -of ts
               -o ${output}
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/po
# This is a workaround to add (duplicate) strings that lconvert missed to the .ts
          COMMAND $<TARGET_PROPERTY:Qt5::lupdate,LOCATION>
          ARGS -silent
               ${CMAKE_SOURCE_DIR}/src/
               -ts ${output}
          DEPENDS ${basename}.po)
  set(${outvar} ${output})
endmacro(generate_ts outvar basename)

# This generates a .qm from a .ts file
macro(generate_qm outvar basename)
  set(input ${CMAKE_BINARY_DIR}/po/${basename}.ts)
  set(output ${CMAKE_BINARY_DIR}/po/${basename}.qm)
  add_custom_command(OUTPUT ${output}
          COMMAND $<TARGET_PROPERTY:Qt5::lrelease,LOCATION>
          ARGS -silent
               ${input}
          DEPENDS ${basename}.ts)
  set(${outvar} ${output})
endmacro(generate_qm outvar basename)

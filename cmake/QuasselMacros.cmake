# This macro sets variables for the Qt modules we need.

macro(setup_qt_variables)
  set(QUASSEL_QT_LIBRARIES )
  set(QUASSEL_QT_INCLUDES ${QT_INCLUDE_DIR})    # Qt4
  set(QUASSEL_QT_DEFINITIONS ${QT_DEFINITIONS}) # Qt4

  IF(WIN32)
    set(MAIN Main)
  ENDIF(WIN32)
  foreach(qtmod Core ${ARGV} ${MAIN})
    if(WITH_QT5)
      find_package(Qt5${qtmod} ${QT_MIN_VERSION} REQUIRED)
      list(APPEND QUASSEL_QT_LIBRARIES ${Qt5${qtmod}_LIBRARIES})
      list(APPEND QUASSEL_QT_INCLUDES ${Qt5${qtmod}_INCLUDE_DIRS})
      list(APPEND QUASSEL_QT_DEFINITIONS ${Qt5${qtmod}_DEFINITIONS} ${Qt5${qtmod}_EXECUTABLE_COMPILE_FLAGS})
    else(WITH_QT5)
      string(TOUPPER ${qtmod} QTMOD)
      list(APPEND QUASSEL_QT_LIBRARIES ${QT_QT${QTMOD}_LIBRARY})
      if(STATIC)
        list(APPEND QUASSEL_QT_LIBRARIES ${QT_QT${QTMOD}_LIB_DEPENDENCIES})
      endif(STATIC)
      list(APPEND QUASSEL_QT_INCLUDES ${QT_QT${QTMOD}_INCLUDE_DIR})
      list(APPEND QUASSEL_QT_DEFINITIONS -DQT_QT${QTMOD}_LIB)
    endif(WITH_QT5)
  endforeach(qtmod)

  list(REMOVE_DUPLICATES QUASSEL_QT_LIBRARIES)
  list(REMOVE_DUPLICATES QUASSEL_QT_INCLUDES)
  list(REMOVE_DUPLICATES QUASSEL_QT_DEFINITIONS)

  # The COMPILE_FLAGS property expects a string, not a list...
  set(QUASSEL_QT_COMPILEFLAGS )
  foreach(flag ${QUASSEL_QT_DEFINITIONS})
    set(QUASSEL_QT_COMPILEFLAGS "${QUASSEL_QT_COMPILEFLAGS} ${flag}")
  endforeach(flag)

endmacro(setup_qt_variables)

# This generates a .ts from a .po file
macro(generate_ts outvar basename)
  set(input ${basename}.po)
  set(output ${CMAKE_BINARY_DIR}/po/${basename}.ts)
  add_custom_command(OUTPUT ${output}
          COMMAND ${QT_LCONVERT_EXECUTABLE}
          ARGS -i ${input}
               -of ts
               -o ${output}
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/po
# This is a workaround to add (duplicate) strings that lconvert missed to the .ts
          COMMAND ${QT_LUPDATE_EXECUTABLE}
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
          COMMAND ${QT_LRELEASE_EXECUTABLE}
          ARGS -silent
               ${input}
          DEPENDS ${basename}.ts)
  set(${outvar} ${output})
endmacro(generate_qm outvar basename)

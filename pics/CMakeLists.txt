# pics/ contains non-themed icons and other graphical resources

if(WIN32)
  if(MINGW)
    find_program(WINDRES_EXECUTABLE NAMES windres)
    if(WINDRES_EXECUTABLE)
      exec_program(windres
              ARGS "-i ${CMAKE_CURRENT_SOURCE_DIR}/win32.rc"
                   "-o ${CMAKE_CURRENT_BINARY_DIR}/win32.o"
                   "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}")
      set(COMMON_DEPS ${COMMON_DEPS} ${CMAKE_CURRENT_BINARY_DIR}/win32.o PARENT_SCOPE)
    endif()
  else()
    set(COMMON_DEPS ${COMMON_DEPS} ${CMAKE_CURRENT_SOURCE_DIR}/win32.rc PARENT_SCOPE)
  endif()
endif()

if (WANT_MONO OR WANT_QTCLIENT)
    quassel_add_resource(Pics PREFIX pics PATTERNS
        kde-vdg.png
        quassel-64.svg
        quassel-eye.png
        quassel-logo.png
        quassel-oxygen-32.png
        qt-logo.png
        qt-logo-32.png
    )
endif()

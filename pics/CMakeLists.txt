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
    endif(WINDRES_EXECUTABLE)
  else(MINGW)
    set(COMMON_DEPS ${COMMON_DEPS} ../pics/win32.rc PARENT_SCOPE)
  endif(MINGW)
endif(WIN32)

if(WANT_MONO OR WANT_QTCLIENT)
  if(EMBED_DATA)
    set(CLIENT_RCS ${CLIENT_RCS} ../pics/pics.qrc PARENT_SCOPE)
  else(EMBED_DATA)
    # We don't find them yet externally, so disable installation
    set(CLIENT_RCS ${CLIENT_RCS} ../pics/pics.qrc PARENT_SCOPE)
    # install(FILES qt-logo.png
    #               quassel-large.png
    #         DESTINATION ${DATA_INSTALL_DIR}/quassel/pics)
  endif(EMBED_DATA)
endif(WANT_MONO OR WANT_QTCLIENT)

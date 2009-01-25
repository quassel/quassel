# pics/ contains non-themed icons and other graphical resources

if(WIN32)
  set(COMMON_DEPS ${COMMON_DEPS} ../pics/win32.rc PARENT_SCOPE)
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

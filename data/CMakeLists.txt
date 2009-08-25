if(NOT WIN32 AND NOT APPLE)
  if(WANT_QTCLIENT)
    install(FILES quasselclient.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
  endif(WANT_QTCLIENT)

  if(WANT_MONO)
    install(FILES quassel.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
  endif(WANT_MONO)

  if(WANT_MONO OR WANT_QTCLIENT)
    if(HAVE_KDE)
      install(FILES quassel.notifyrc DESTINATION ${DATA_INSTALL_DIR}/quassel)
    endif(HAVE_KDE)
  endif(WANT_MONO OR WANT_QTCLIENT)
endif(NOT WIN32 AND NOT APPLE)

if(WANT_MONO OR WANT_QTCLIENT)
  if(EMBED_DATA)
    set(CLIENT_RCS ${CLIENT_RCS} ../data/data.qrc PARENT_SCOPE)
  else(EMBED_DATA)
    install(FILES networks.ini DESTINATION ${DATA_INSTALL_DIR}/quassel)
    install(DIRECTORY stylesheets DESTINATION ${DATA_INSTALL_DIR}/quassel)
  endif(EMBED_DATA)

  if(NOT WIN32)
    install(DIRECTORY scripts DESTINATION ${DATA_INSTALL_DIR}/quassel USE_SOURCE_PERMISSIONS)
  endif(NOT WIN32)

endif(WANT_MONO OR WANT_QTCLIENT)

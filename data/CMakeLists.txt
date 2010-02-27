# cmake-2.6.2 can't handle nested conditions
# if((UNIX AND NOT APPLE) OR HAVE_KDE)
if(HAVE_KDE OR UNIX)
 if(HAVE_KDE OR NOT APPLE)
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

# endif((UNIX AND NOT APPLE) OR HAVE_KDE)
 endif(HAVE_KDE OR NOT APPLE)
endif(HAVE_KDE OR UNIX)

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

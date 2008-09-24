if(WANT_QTCLIENT)
  install(FILES quasselclient.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
endif(WANT_QTCLIENT)

if(WANT_MONO)
  install(FILES quassel.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
endif(WANT_MONO)

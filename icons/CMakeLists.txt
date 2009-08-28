# Install icons
# We put them in DATA_INSTALL_DIR rather than ICON_INSTALL_DIR, to avoid
# polluting the global namespace and to allow overriding

if(WANT_MONO OR WANT_QTCLIENT)
  if(EMBED_DATA)
    set(CLIENT_RCS ${CLIENT_RCS} ../icons/hicolor.qrc ../icons/oxygen.qrc PARENT_SCOPE)
  else(EMBED_DATA)
    install(DIRECTORY hicolor DESTINATION ${DATA_INSTALL_DIR}/quassel/icons)
    install(DIRECTORY oxygen DESTINATION ${DATA_INSTALL_DIR}/quassel/icons)
  endif(EMBED_DATA)
endif(WANT_MONO OR WANT_QTCLIENT)

# Application icon
if(NOT APPLE AND NOT WIN32)
  install(FILES oxygen/48x48/apps/quassel.png DESTINATION ${ICON_INSTALL_DIR}/hicolor/48x48/apps)
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    install(FILES oxygen/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
  endif(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
endif(NOT APPLE AND NOT WIN32)

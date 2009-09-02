# Install icons
# We put them in DATA_INSTALL_DIR rather than ICON_INSTALL_DIR, to avoid
# polluting the global namespace and to allow overriding

if(WANT_MONO OR WANT_QTCLIENT)

  # Figure out if we want to install KDE's icons
  # Starting with KDE 4.3, they shouldn't be needed anymore
  string(TOUPPER ${WITH_OXYGEN} OXY_UPPER)
  if(NOT OXY_UPPER)
    set(INSTALL_OXY false)
  elseif(OXY_UPPER MATCHES "AUTO")
    if(HAVE_KDE AND KDE_VERSION VERSION_GREATER 4.2.99)
      set(INSTALL_OXY false)
    else(HAVE_KDE AND KDE_VERSION VERSION_GREATER 4.2.99)
      set(INSTALL_OXY true)
    endif(HAVE_KDE AND KDE_VERSION VERSION_GREATER 4.2.99)
  else(NOT OXY_UPPER)
    set(INSTALL_OXY true)
  endif(NOT OXY_UPPER)

  if(INSTALL_OXY)
    message(STATUS "Installing Oxygen icons")
  else(INSTALL_OXY)
    message(STATUS "Not installing Oxygen icons")
  endif(INSTALL_OXY)

  if(EMBED_DATA)
    set(ICON_RCS ../icons/hicolor.qrc ../icons/oxygen.qrc)
    if(INSTALL_OXY)
      set(ICON_RCS ${ICON_RCS} ../icons/oxygen_kde.qrc)
    endif(INSTALL_OXY)
    set(CLIENT_RCS ${CLIENT_RCS} ${ICON_RCS} PARENT_SCOPE)
  else(EMBED_DATA)
    install(DIRECTORY hicolor DESTINATION ${DATA_INSTALL_DIR}/quassel/icons)
    install(DIRECTORY oxygen DESTINATION ${DATA_INSTALL_DIR}/quassel/icons)
    if(INSTALL_OXY)
      install(DIRECTORY oxygen_kde/ DESTINATION ${DATA_INSTALL_DIR}/quassel/icons/oxygen)
    endif(INSTALL_OXY)
  endif(EMBED_DATA)
endif(WANT_MONO OR WANT_QTCLIENT)

# Application icon
if(NOT APPLE AND NOT WIN32)
  install(FILES oxygen_kde/48x48/apps/quassel.png DESTINATION ${ICON_INSTALL_DIR}/hicolor/48x48/apps)
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    install(FILES oxygen_kde/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
  endif(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
endif(NOT APPLE AND NOT WIN32)

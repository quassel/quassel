# Install icons
# We put them in DATA_INSTALL_DIR rather than ICON_INSTALL_DIR, to avoid
# polluting the global namespace and to allow overriding

if(WANT_MONO OR WANT_QTCLIENT)

  # Figure out if we want to install KDE's icons
  # Starting with KDE 4.3, they shouldn't be needed anymore
  set(INSTALL_OXY true)
  string(TOUPPER ${WITH_OXYGEN} OXY_UPPER)
  if(NOT OXY_UPPER)
    set(INSTALL_OXY false)
  elseif(OXY_UPPER MATCHES "AUTO")
    if(HAVE_KDE)
      if(KDE_VERSION VERSION_GREATER 4.2.99)
        set(INSTALL_OXY false)
      endif(KDE_VERSION VERSION_GREATER 4.2.99)
    endif(HAVE_KDE)
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
    install(DIRECTORY hicolor DESTINATION ${ICON_INSTALL_DIR})
    install(DIRECTORY oxygen DESTINATION ${DATA_INSTALL_DIR}/quassel/icons)
    if(INSTALL_OXY)
      install(DIRECTORY oxygen_kde/ DESTINATION ${DATA_INSTALL_DIR}/quassel/icons/oxygen)
    endif(INSTALL_OXY)
  endif(EMBED_DATA)
endif(WANT_MONO OR WANT_QTCLIENT)

# Application icon

# cmake-2.6.2 can't handle nested conditions
# if((UNIX AND NOT APPLE) OR HAVE_KDE)
if(HAVE_KDE OR UNIX)
 if(HAVE_KDE OR NOT APPLE)

  install(FILES oxygen_kde/48x48/apps/quassel.png DESTINATION ${ICON_INSTALL_DIR}/hicolor/48x48/apps)
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    install(FILES oxygen_kde/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
  endif(CMAKE_INSTALL_PREFIX STREQUAL "/usr")

# endif((UNIX AND NOT APPLE) OR HAVE_KDE)
 endif(HAVE_KDE OR NOT APPLE)
endif(HAVE_KDE OR UNIX)

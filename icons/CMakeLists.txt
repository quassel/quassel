# Build and/or install icons according to global settings

if(QUASSEL_ICONS MATCHES "External")
  install(DIRECTORY hicolor DESTINATION ${ICON_INSTALL_DIR})
else(QUASSEL_ICONS MATCHES "External")
  qt4_add_resources(RC_ICONS hicolor.qrc)
endif(QUASSEL_ICONS MATCHES "External")

if(OXYGEN_ICONS MATCHES "Builtin")
  qt4_add_resources(RC_ICONS oxygen.qrc)
endif(OXYGEN_ICONS MATCHES "Builtin")

# Application icon
if(NOT APPLE AND NOT WIN32)
  install(FILES hicolor/48x48/apps/quassel.png DESTINATION ${ICON_INSTALL_DIR}/hicolor/48x48/apps)
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    install(FILES hicolor/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
  endif(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
endif(NOT APPLE AND NOT WIN32)

add_custom_target(icons DEPENDS ${RC_ICONS})

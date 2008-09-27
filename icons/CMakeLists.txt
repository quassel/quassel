# Build and/or install icons according to global settings

if(QUASSEL_ICONS MATCHES "External")
  install(DIRECTORY hicolor DESTINATION ${ICON_INSTALL_DIR})
else(QUASSEL_ICONS MATCHES "External")
  qt4_add_resources(RC_ICONS hicolor.qrc)
endif(QUASSEL_ICONS MATCHES "External")

if(OXYGEN_ICONS MATCHES "Builtin")
  qt4_add_resources(RC_ICONS oxygen.qrc)
endif(OXYGEN_ICONS MATCHES "Builtin")

add_custom_target(icons DEPENDS ${RC_ICONS})

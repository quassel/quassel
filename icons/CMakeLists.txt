# Build and/or install icons according to global settings

if(QUASSEL_ICONS MATCHES "External")
  install(DIRECTORY hicolor DESTINATION ${ICON_INSTALL_DIR})
else(QUASSEL_ICONS MATCHES "External")
  set(CLIENT_DEPS ${CLIENT_DEPS} hicolor.qrc)
endif(QUASSEL_ICONS MATCHES "External")

if(OXYGEN_ICONS MATCHES "Builtin")
  set(CLIENT_DEPS ${CLIENT_DEPS} oxygen.qrc)
endif(OXYGEN_ICONS MATCHES "Builtin")

# Builds the three main targets

add_subdirectory(common)
include_directories(common)
if(WANT_CORE OR WANT_MONO)
  add_subdirectory(core)
  include_directories(core)
endif(WANT_CORE OR WANT_MONO)
if(WANT_QTCLIENT OR WANT_MONO)
  add_subdirectory(client)
  add_subdirectory(uisupport)
  add_subdirectory(qtui)
  include_directories(client)
  include_directories(uisupport)
  include_directories(qtui)
endif(WANT_QTCLIENT OR WANT_MONO)

# We always add stuff in :/pics
qt4_add_resources(CLIENT_DEPS ../pics/pics.qrc)

if(QUASSEL_ICONS MATCHES "Builtin")
  set(CLIENT_DEPS ${CLIENT_DEPS} ../icons/qrc_hicolor.cxx)
  set_source_files_properties(../icons/qrc_hicolor.cxx PROPERTIES GENERATED true)
endif(QUASSEL_ICONS MATCHES "Builtin")

if(OXYGEN_ICONS MATCHES "Builtin")
  set(CLIENT_DEPS ${CLIENT_DEPS} ../icons/qrc_oxygen.cxx)
  set_source_files_properties(../icons/qrc_oxygen.cxx PROPERTIES GENERATED true)
endif(OXYGEN_ICONS MATCHES "Builtin")

if(WANT_CORE)
  setup_qt4_variables(NETWORK SCRIPT SQL)
  add_executable(quasselcore common/main.cpp ${COMMON_DEPS} ${CORE_DEPS})
  set_target_properties(quasselcore PROPERTIES
                                    COMPILE_FLAGS "-DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_CORE"
                                    OUTPUT_NAME ../quasselcore)
  target_link_libraries(quasselcore mod_core mod_common
                                    ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
  install(TARGETS quasselcore RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_CORE)

if(WANT_QTCLIENT)
  setup_qt4_variables(${LINK_DBUS} GUI NETWORK ${LINK_WEBKIT})
  add_executable(quasselclient WIN32 common/main.cpp ${COMMON_DEPS} ${CLIENT_DEPS})
  add_dependencies(quasselclient icons)
  set_target_properties(quasselclient PROPERTIES
                                      COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DBUILD_QTUI"
                                      OUTPUT_NAME ../quasselclient)
  target_link_libraries(quasselclient mod_qtui mod_uisupport mod_client mod_common
                                      ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
  install(TARGETS quasselclient RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_QTCLIENT)

if(WANT_MONO)
  setup_qt4_variables(${LINK_DBUS} GUI NETWORK SCRIPT SQL ${LINK_WEBKIT})
  add_executable(quassel WIN32 common/main.cpp qtui/monoapplication.cpp ${COMMON_DEPS} ${CLIENT_DEPS} ${CORE_DEPS})
  add_dependencies(quassel icons)
  set_target_properties(quassel PROPERTIES
                                COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_MONO"
                                OUTPUT_NAME ../quassel)
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common
                                ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
  install(TARGETS quassel RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_MONO)

# Build bundles for MacOSX
if(APPLE)
  add_custom_command(TARGET quasselclient POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makebundle.py
                             ${CMAKE_SOURCE_DIR} "Quassel Client" ../quasselclient)
  add_custom_command(TARGET quassel POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makebundle.py
                             ${CMAKE_SOURCE_DIR} "Quassel" ../quassel)
  if(DEPLOY)
    add_custom_command(TARGET quasselclient POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Client ..)
    add_custom_command(TARGET quasselcore POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Core ..)
  endif(DEPLOY)
endif(APPLE)

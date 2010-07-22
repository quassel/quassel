# Builds the three main targets

add_subdirectory(common)
include_directories(BEFORE common)
if(WANT_CORE OR WANT_MONO)
  add_subdirectory(core)
  include_directories(BEFORE core)
endif(WANT_CORE OR WANT_MONO)
if(WANT_QTCLIENT OR WANT_MONO)
  add_subdirectory(client)
  add_subdirectory(uisupport)
  add_subdirectory(qtui)
  include_directories(BEFORE client)
  include_directories(BEFORE uisupport)
  include_directories(BEFORE qtui)
#  if(HAVE_KDE)
#    add_subdirectory(kdeui)
#    include_directories(kdeui)
#    set(LINK_KDE mod_kdeui)
#  endif(HAVE_KDE)

  if(STATIC)
    link_directories(${QT_PLUGINS_DIR}/imageformats)
    set(CLIENT_LIBS ${CLIENT_LIBS} qjpeg qgif)
  endif(STATIC)

endif(WANT_QTCLIENT OR WANT_MONO)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}) # for version.gen

# Add resources. Can't be done in other subdirs apparently.
# Note that these variables need to contain paths relative to src/ (this dir)
qt4_add_resources(CLIENT_DEPS ${CLIENT_RCS})
qt4_add_resources(CORE_DEPS ${CORE_RCS})
qt4_add_resources(COMMON_DEPS ${COMMON_RCS})

if(HAVE_INDICATEQT)
  link_directories(${INDICATEQT_LIBRARY_DIRS})
endif(HAVE_INDICATEQT)

if(WANT_CORE)
  setup_qt4_variables(NETWORK SCRIPT SQL)
  add_executable(quasselcore common/main.cpp ${COMMON_DEPS} ${CORE_DEPS})
  add_dependencies(quasselcore po genversion_run)
  set_target_properties(quasselcore PROPERTIES
                                    COMPILE_FLAGS "-DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_CORE"
                                    OUTPUT_NAME ../quasselcore)
  target_link_libraries(quasselcore mod_core mod_common
                                    ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
  install(TARGETS quasselcore RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_CORE)

if(WANT_QTCLIENT)
  setup_qt4_variables(GUI NETWORK ${CLIENT_QT4_VARS})
  add_executable(quasselclient WIN32 common/main.cpp ${COMMON_DEPS} ${CLIENT_DEPS})
  add_dependencies(quasselclient po genversion_run)
  set_target_properties(quasselclient PROPERTIES
                                      COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB ${CLIENT_COMPILE_FLAGS} -DBUILD_QTUI"
                                      OUTPUT_NAME ../quasselclient)
  target_link_libraries(quasselclient ${LINK_KDE} mod_qtui mod_uisupport mod_client mod_common ${CLIENT_LIBS}
                                      ${QUASSEL_QT_LIBRARIES} ${QUASSEL_KDE_LIBRARIES} ${QUASSEL_SSL_LIBRARIES} ${PHONON_LIBS} ${INDICATEQT_LIBRARIES})
  install(TARGETS quasselclient RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_QTCLIENT)

if(WANT_MONO)
  setup_qt4_variables(GUI NETWORK SCRIPT SQL ${CLIENT_QT4_VARS})
  qt4_wrap_cpp(MOC qtui/monoapplication.h)
  add_executable(quassel WIN32 common/main.cpp qtui/monoapplication.cpp ${MOC} ${COMMON_DEPS} ${CLIENT_DEPS} ${CORE_DEPS})
  add_dependencies(quassel po genversion_run)
  set_target_properties(quassel PROPERTIES
                                COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB ${CLIENT_COMPILE_FLAGS} -DBUILD_MONO"
                                OUTPUT_NAME ../quassel)
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common ${CLIENT_LIBS}
                                ${QUASSEL_QT_LIBRARIES} ${QUASSEL_KDE_LIBRARIES} ${QUASSEL_SSL_LIBRARIES} ${PHONON_LIBS} ${INDICATEQT_LIBRARIES})
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

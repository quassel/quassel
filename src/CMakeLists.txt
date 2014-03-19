# Builds the three main targets

add_subdirectory(common)
include_directories(BEFORE common)
if(BUILD_CORE)
  add_subdirectory(core)
  include_directories(BEFORE core)
endif(BUILD_CORE)
if(BUILD_GUI)
  add_subdirectory(client)
  add_subdirectory(uisupport)
  add_subdirectory(qtui)
  include_directories(BEFORE client)
  include_directories(BEFORE uisupport)
  include_directories(BEFORE qtui)

  if(STATIC)
    link_directories(${QT_PLUGINS_DIR}/imageformats)
    set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} qjpeg qgif)
  endif(STATIC)
endif(BUILD_GUI)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}) # for version.gen

# Add resources. Can't be done in other subdirs apparently.
# Note that these variables need to contain paths relative to src/ (this dir)
qt_add_resources(CLIENT_DEPS ${CLIENT_RCS})
qt_add_resources(CORE_DEPS ${CORE_RCS})
qt_add_resources(COMMON_DEPS ${COMMON_RCS})

# For Windows, Qt4 needs the QtMain module
if (WIN32 AND NOT WITH_QT5)
    set(MAIN Main)
endif()

if(WANT_CORE)
  add_executable(quasselcore common/main.cpp ${COMMON_DEPS} ${CORE_DEPS})
  qt_use_modules(quasselcore Core Network ${CORE_QT_MODULES} ${MAIN})
  add_dependencies(quasselcore po)
  set_target_properties(quasselcore PROPERTIES
                                    COMPILE_FLAGS "-DBUILD_CORE"
                                    OUTPUT_NAME ../quasselcore)
  target_link_libraries(quasselcore mod_core mod_common ${COMMON_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
  install(TARGETS quasselcore RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_CORE)

if(WANT_QTCLIENT)
  add_executable(quasselclient WIN32 common/main.cpp ${COMMON_DEPS} ${CLIENT_DEPS})
  qt_use_modules(quasselclient Core Gui Network ${CLIENT_QT_MODULES} ${MAIN})
  add_dependencies(quasselclient po)
  set_target_properties(quasselclient PROPERTIES
                                      COMPILE_FLAGS "-DBUILD_QTUI ${CLIENT_COMPILE_FLAGS}"
                                      OUTPUT_NAME ../quasselclient)
  target_link_libraries(quasselclient ${LINK_KDE} mod_qtui mod_uisupport mod_client mod_common ${COMMON_LIBRARIES} ${CLIENT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
  install(TARGETS quasselclient RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif(WANT_QTCLIENT)

if(WANT_MONO)
  add_executable(quassel WIN32 common/main.cpp qtui/monoapplication.cpp ${COMMON_DEPS} ${CLIENT_DEPS} ${CORE_DEPS})
  qt_use_modules(quassel Core Gui Network ${CLIENT_QT_MODULES} ${CORE_QT_MODULES} ${MAIN})
  add_dependencies(quassel po)
  set_target_properties(quassel PROPERTIES
                                COMPILE_FLAGS "-DBUILD_MONO ${CLIENT_COMPILE_FLAGS}"
                                OUTPUT_NAME ../quassel)
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common ${COMMON_LIBRARIES} ${CLIENT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
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

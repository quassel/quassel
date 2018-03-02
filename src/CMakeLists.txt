# Builds the three main targets

# We need to initialize the appropriate resources, so let's give our main.cpp some hints
if (EMBED_DATA)
    add_definitions(-DEMBED_DATA)
endif()

add_subdirectory(common)
include_directories(BEFORE common)
if(BUILD_CORE)
  add_subdirectory(core)
  include_directories(BEFORE core)
endif()
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
  endif()
endif()

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}) # for version.gen

# For KAboutData
if (WITH_KF5)
    set(CLIENT_LIBRARIES ${CLIENT_LIBRARIES} KF5::CoreAddons)
endif()

# Needed for showing the cli option if appropriate
if (HAVE_SYSLOG)
    add_definitions(-DHAVE_SYSLOG)
endif()

if(WANT_CORE)
  add_executable(quasselcore common/main.cpp ${CORE_DEPS} ${COMMON_DEPS})
  qt_use_modules(quasselcore Core Network ${CORE_QT_MODULES})
  add_dependencies(quasselcore po)
  set_target_properties(quasselcore PROPERTIES
                                    COMPILE_FLAGS "-DBUILD_CORE"
                                    OUTPUT_NAME ../quasselcore)
  target_link_libraries(quasselcore mod_core mod_common ${COMMON_LIBRARIES} ${QUASSEL_SSL_LIBRARIES} ${QT_QTMAIN_LIBRARY})
  install(TARGETS quasselcore RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (KDE4_FOUND)
    include_directories(${KDE4_INCLUDES})
endif()

if(WANT_QTCLIENT)
  add_executable(quasselclient WIN32 common/main.cpp ${CLIENT_DEPS} ${COMMON_DEPS})
  qt_use_modules(quasselclient Core Gui Network ${CLIENT_QT_MODULES})
  add_dependencies(quasselclient po)
  set_target_properties(quasselclient PROPERTIES
                                      COMPILE_FLAGS "-DBUILD_QTUI"
                                      OUTPUT_NAME ../quasselclient)
  target_link_libraries(quasselclient mod_qtui mod_uisupport mod_client mod_common ${COMMON_LIBRARIES} ${CLIENT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES} ${QT_QTMAIN_LIBRARY})
  install(TARGETS quasselclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(WANT_MONO)
  add_executable(quassel WIN32 common/main.cpp qtui/monoapplication.cpp ${CLIENT_DEPS} ${CORE_DEPS} ${COMMON_DEPS})
  qt_use_modules(quassel Core Gui Network ${CLIENT_QT_MODULES} ${CORE_QT_MODULES})
  add_dependencies(quassel po)
  set_target_properties(quassel PROPERTIES
                                COMPILE_FLAGS "-DBUILD_MONO"
                                OUTPUT_NAME ../quassel)
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common ${COMMON_LIBRARIES} ${CLIENT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES} ${QT_QTMAIN_LIBRARY})
  install(TARGETS quassel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

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
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Client .. qsvgicon)
    add_custom_command(TARGET quasselcore POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Core ..)
    add_custom_command(TARGET quassel POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Mono .. qsvgicon)
  endif()
endif()

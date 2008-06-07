# This is the cmake-based build system for Quassel IRC.
# You may pass various options to cmake:
# -DBUILD=<string> : Select binaries to build. <string> may contain any combination
#                    of "core", "client", "mono" or "all".
# -DQT=/path/to/qt : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=1       : Enable static building of Quassel, most useful with a static Qt.
# -DSPUTDEV        : Do not use.
#
# NOTE: You need to remove CMakeCache.txt if you plan to change any of these values!

project(QuasselIRC)

cmake_minimum_required(VERSION 2.4.5)

set(QT_MIN_VERSION "4.4.0")

# By default, we build all binaries
if(NOT DEFINED BUILD)
  set(BUILD all)
endif(NOT DEFINED BUILD)

# User might define which binaries to build by invoking cmake -DBUILD=<string>,
# where <string> might contain any combination of "core", "client", "mono" or "all"
if(BUILD MATCHES all)
  set(BUILD_CORE true)
  set(BUILD_QTCLIENT true)
  set(BUILD_MONO true)
  message(STATUS "Building Quassel Client, Quassel Core and monolithic Quassel.")
else(BUILD MATCHES all)
  if(BUILD MATCHES core)
    set(BUILD_CORE true)
    message(STATUS "Building Quassel Core")
  endif(BUILD MATCHES core)
  if(BUILD MATCHES client)
    set(BUILD_QTCLIENT true)
    message(STATUS "Building Quassel Client")
  endif(BUILD MATCHES client)
  if(BUILD MATCHES mono)
    set(BUILD_MONO true)
    message(STATUS "Building monolithic Quassel")
  endif(BUILD MATCHES mono)
endif(BUILD MATCHES all)

# Select a Qt installation here, if you don't want to use system Qt
if(DEFINED QT)
  # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
  set(ENV{PATH} ${QT}/bin:$ENV{PATH})
  #SET(QT_QMAKE_EXECUTABLE ${QT}/bin/qmake CACHE FILEPATH "" FORCE)
endif(DEFINED QT)

# Enable mostly b0rked stuff (new ChatView), do not enable this unless you know what you do...
if(SPUTDEV)
  add_definitions(-DSPUTDEV)
endif(SPUTDEV)

# Now that we have the correct $PATH, lets find Qt!
find_package(Qt4 REQUIRED)

# Add needed subdirs
add_subdirectory(src/common)
include_directories(src/common)
if(BUILD_CORE OR BUILD_MONO)
  add_subdirectory(src/core)
  include_directories(src/core)
endif(BUILD_CORE OR BUILD_MONO)
if(BUILD_QTCLIENT OR BUILD_MONO)
  add_subdirectory(src/client)
  add_subdirectory(src/uisupport)
  add_subdirectory(src/qtui)
  include_directories(src/client)
  include_directories(src/uisupport)
  include_directories(src/qtui)
endif(BUILD_QTCLIENT OR BUILD_MONO)

# Add resources
qt4_add_resources(RES_I18N ${CMAKE_SOURCE_DIR}/i18n/i18n.qrc)
qt4_add_resources(RES_ICONS ${CMAKE_SOURCE_DIR}/src/icons/icons.qrc)
qt4_add_resources(RES_SQL ${CMAKE_SOURCE_DIR}/src/core/sql.qrc)

# Set global buildflags
if(DEFINED STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc ${CMAKE_EXE_LINKER_FLAGS}")
  link_directories(${CMAKE_BINARY_DIR}/staticlibs)
endif(DEFINED STATIC)

# Here comes the dirty part. Our targets need different Qt4 modules, i.e. different libs
# and defines. We can't simply include UseQt4 several times, since definitions add up.
# We workaround this by only setting up QtCore first, and adding additional stuff later.
set(QT_DONT_USE_QTGUI 1)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDES})

# This macro sets variables for additional Qt modules.
macro(setup_qt4_variables)
  set(QUASSEL_QT_DEFINITIONS ${QT_DEFINITIONS})
  set(QUASSEL_QT_LIBRARIES )
  foreach(qtmod ${ARGV})
    # This needs to be a string, not a list, otherwise set_target_properties screws up...
    set(QUASSEL_QT_DEFINITIONS "${QUASSEL_QT_DEFINITIONS} -DQT_${qtmod}_LIB")
    set(QUASSEL_QT_LIBRARIES ${QUASSEL_QT_LIBRARIES} ${QT_QT${qtmod}_LIBRARY} ${QT_${qtmod}_LIB_DEPENDENCIES})
  endforeach(qtmod ${ARGV})
  set(QUASSEL_QT_LIBRARIES ${QUASSEL_QT_LIBRARIES} ${QT_LIBRARIES})
endmacro(setup_qt4_variables)

# Now we have everything, so just glue the right pieces together :)
if(BUILD_CORE)
  setup_qt4_variables(NETWORK SCRIPT SQL)
  add_executable(quasselcore ${CMAKE_SOURCE_DIR}/src/common/main.cpp ${RES_SQL} ${RES_I18N})
  set_target_properties(quasselcore PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_DEFINITIONS} -DBUILD_CORE")
  target_link_libraries(quasselcore mod_core mod_common ${QUASSEL_QT_LIBRARIES})
endif(BUILD_CORE)

if(BUILD_QTCLIENT)
  setup_qt4_variables(GUI NETWORK)
  add_executable(quasselclient ${CMAKE_SOURCE_DIR}/src/common/main.cpp ${RES_ICONS} ${RES_I18N})
  set_target_properties(quasselclient PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_DEFINITIONS} -DBUILD_QTUI")
  target_link_libraries(quasselclient mod_qtui mod_uisupport mod_client mod_common ${QUASSEL_QT_LIBRARIES})
endif(BUILD_QTCLIENT)

if(BUILD_MONO)
  setup_qt4_variables(GUI NETWORK SCRIPT SQL)
  add_executable(quassel ${CMAKE_SOURCE_DIR}/src/common/main.cpp ${RES_ICONS} ${RES_SQL} ${RES_I18N})
  set_target_properties(quassel PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_DEFINITIONS} -DBUILD_MONO")
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common ${QUASSEL_QT_LIBRARIES})
endif(BUILD_MONO)

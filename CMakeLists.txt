# This is the cmake-based build system for Quassel IRC.
# You may pass various options to cmake:
# -DBUILD=<string> : Select binaries to build. <string> may contain any combination
#                    of "core", "client", "mono" or "all".
# -DQT=/path/to/qt : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=1       : Enable static building of Quassel, most useful with a static Qt.
# -DSTATICWIN=1    : Enable static building for Windows.
# -DSPUTDEV        : Do not use.
#
# NOTE: You need to remove CMakeCache.txt if you plan to change any of these values!

project(QuasselIRC)

cmake_minimum_required(VERSION 2.4.5)

# This would suppress annoying warnings on cmake-2.6, but we can't use it 
# with 2.4, so... DUH!
# cmake_policy(SET CMP0003 OLD)  # suppress linker warnings

if(STATICWIN)
  set(CMAKE_BUILD_TYPE Release)
endif(STATICWIN)

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

# Enable mostly b0rked stuff (new ChatView), do not enable this unless you know what you do...
if(SPUTDEV)
  add_definitions(-DSPUTDEV)
endif(SPUTDEV)

# Set up OpenSSL
find_package(OpenSSL)

# Select a Qt installation here, if you don't want to use system Qt
if(DEFINED QT)
  # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
  set(ENV{PATH} ${QT}/bin:$ENV{PATH})
  #SET(QT_QMAKE_EXECUTABLE ${QT}/bin/qmake CACHE FILEPATH "" FORCE)
endif(DEFINED QT)

# Now that we have the correct $PATH, lets find Qt!
find_package(Qt4 REQUIRED)

set(QT_DONT_USE_QTGUI 1)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDES})

# We need to create a version.gen
# For this, we create our genversion binary and make sure it is run every time.
add_executable(genversion ${CMAKE_SOURCE_DIR}/src/common/genversion.cpp)
target_link_libraries(genversion ${QT_LIBRARIES})

add_custom_target(genversion_run ALL ${CMAKE_BINARY_DIR}/genversion
                  ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/src/common/version.gen)
add_dependencies(genversion_run genversion)

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

# Make sure version.gen exists before building mod_common
add_dependencies(mod_common genversion_run)

# Add resources
qt4_add_resources(RC_I18N i18n/i18n.qrc)
qt4_add_resources(RC_ICONS src/icons/icons.qrc)
qt4_add_resources(RC_QUASSEL_ICONS src/icons/quassel-icons.qrc)
qt4_add_resources(RC_SQL src/core/sql.qrc)

# Set global buildflags
if(DEFINED STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc ${CMAKE_EXE_LINKER_FLAGS}")
  link_directories(${CMAKE_BINARY_DIR}/staticlibs)
endif(DEFINED STATIC)

if(STATICWIN AND WIN32)
  link_libraries(imm32 winmm)  # missing by default :/
   if(OPENSSL_FOUND)
     link_libraries(${OPENSSL_LIBRARIES} libeay32MD)
   endif(OPENSSL_FOUND)
endif(STATICWIN AND WIN32)

if(WIN32)
  set(WIN32_RC src/icons/win32.rc)  # for app icons on windows
endif(WIN32)

# FIXME: Add icons for windows as soon as we have merged with trunk (which has reorganized
#        the icon files):
#        Simply add the .rc file to the targets

# Here comes the dirty part. Our targets need different Qt4 modules, i.e. different libs
# and defines. We can't simply include UseQt4 several times, since definitions add up.
# We workaround this by using our own macro to figure out what to add.

# This macro sets variables for additional Qt modules.
macro(setup_qt4_variables)
  set(QUASSEL_QT_DEFINITIONS ${QT_DEFINITIONS})
  set(QUASSEL_QT_LIBRARIES )
  foreach(qtmod CORE ${ARGV})
    # This needs to be a string, not a list, otherwise set_target_properties screws up...
    set(QUASSEL_QT_DEFINITIONS "${QUASSEL_QT_DEFINITIONS} -DQT_${qtmod}_LIB")
    set(QUASSEL_QT_LIBRARIES ${QUASSEL_QT_LIBRARIES} ${QT_QT${qtmod}_LIBRARY} ${QT_${qtmod}_LIB_DEPENDENCIES})
  endforeach(qtmod ${ARGV})
  set(QUASSEL_QT_LIBRARIES ${QUASSEL_QT_LIBRARIES} ${QT_LIBRARIES})
endmacro(setup_qt4_variables)

# Now we have everything, so just glue the right pieces together :)
if(BUILD_CORE)
  setup_qt4_variables(NETWORK SCRIPT SQL)
  add_executable(quasselcore ${CMAKE_SOURCE_DIR}/src/common/main.cpp
                             ${RC_SQL} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quasselcore PROPERTIES COMPILE_FLAGS "-DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_CORE")
  target_link_libraries(quasselcore mod_core mod_common ${QUASSEL_QT_LIBRARIES})
endif(BUILD_CORE)

if(BUILD_QTCLIENT)
  setup_qt4_variables(GUI NETWORK)
  add_executable(quasselclient ${CMAKE_SOURCE_DIR}/src/common/main.cpp
                               ${RC_ICONS} ${RC_QUASSEL_ICONS} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quasselclient PROPERTIES COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DBUILD_QTUI")
  target_link_libraries(quasselclient mod_qtui mod_uisupport mod_client mod_common ${QUASSEL_QT_LIBRARIES})
endif(BUILD_QTCLIENT)

if(BUILD_MONO)
  setup_qt4_variables(GUI NETWORK SCRIPT SQL)
  add_executable(quassel ${CMAKE_SOURCE_DIR}/src/common/main.cpp
                         ${RC_ICONS} ${RC_QUASSEL_ICONS} ${RC_SQL} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quassel PROPERTIES COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_MONO")
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common ${QUASSEL_QT_LIBRARIES})
endif(BUILD_MONO)

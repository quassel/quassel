# This is the cmake-based build system for Quassel IRC.
# You may pass various options to cmake:
# -DWANT_(CORE|QTCLIENT|MONO)=(ON|OFF)
#                     : select binaries to build
# -DWITH_OPENSSL=OFF  : Disable OpenSSL support
# -DWITH_DBUS=OFF     : Disable D-Bus support
# -DQT=/path/to/qt    : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=ON         : Enable static building of Quassel. Use with care.
# -DSPUTDEV=ON        : Do not use.
# -DDEPLOY=ON         : Mac OS X only. Use only fore redistribution Quassel Packages!!
#
# NOTE: You need to remove CMakeCache.txt if you plan to change any of these values!

project(QuasselIRC)

# Target scopes don't work in older versions
cmake_minimum_required(VERSION 2.4.7 FATAL_ERROR)

if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Use our own (well, KDE's) version of some modules
# In particular cmake's FindQt4 and FindOpenSSL are quite buggy
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Various options and variables that can be set on the command line
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the Qt4 GUI client binary"          ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" OFF)

option(WITH_OPENSSL  "Enable OpenSSL support if present on the system"	ON)
option(WITH_DBUS     "Enable D-Bus support if present on the system"	ON)

option(STATIC        "Enable static building (might not be portable)" OFF)
option(DEPLOY        "Mac OS X only! Adds required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF)
option(SPUTDEV       "Do not use!" OFF)

set(QT "" CACHE STRING "Path to a Qt installation to use instead of the system Qt")
set(LINGUAS "" CACHE STRING "Space-separated List of locales specifying languages that should be compiled")

if(STATIC)
  set(CMAKE_BUILD_TYPE Release)
endif(STATIC)

# RPATH needs to be set correctly
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1) 
set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# Enable various flags on gcc
if(CMAKE_COMPILER_IS_GNUCXX)
  # Let's just hope that all gccs support these options and skip the tests...
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi -Wall -Wextra -Wnon-virtual-dtor")
endif(CMAKE_COMPILER_IS_GNUCXX)

set(QT_MIN_VERSION "4.4.0")

if(APPLE AND DEPLOY)
  set(CMAKE_OSX_ARCHITECTURES "i386;ppc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4")
  set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
  add_definitions(-DMAC_10_4_SDK)
endif(APPLE AND DEPLOY)

# Enable mostly b0rked stuff (new ChatView), do not enable this unless you know what you do...
if(SPUTDEV)
  add_definitions(-DSPUTDEV)
endif(SPUTDEV)

# Set up execinfo

# The problem with this library is that it is built-in in the Linux glib, 
# while on systems like FreeBSD, it is installed separately and thus needs to be linked to.
# Therefore, we search for the header to see if the it's available in the first place.
# If it is available, we try to locate the library to figure out whether it is built-in or not.

find_path(EXECINFO_H_DIR "execinfo.h")

if(NOT EXECINFO_H_DIR STREQUAL "EXECINFO_H_DIR-NOTFOUND")
  # We found the header file's include dir.
  # Let's add it so the compiler finds it as well.
  include_directories(${EXECINFO_H_DIR})

  # Now determine if it's built-in or not, by searching the library file.
  find_library(EXECINFO_LIB_PATH "execinfo")

  if(EXECINFO_LIB_PATH STREQUAL "EXECINFO_LIB_PATH-NOTFOUND")
    # Built-in, no further action is needed
    message(STATUS "Found execinfo")
  else(EXECINFO_LIB_PATH STREQUAL "EXECINFO_LIB_PATH-NOTFOUND")
    # It's an external library, link it.
    link_libraries(${EXECINFO_LIB_PATH})
    message(STATUS "Found execinfo: ${EXECINFO_LIB_PATH}")
  endif(EXECINFO_LIB_PATH STREQUAL "EXECINFO_LIB_PATH-NOTFOUND")

  set(HAVE_EXECINFO true)

endif(NOT EXECINFO_H_DIR STREQUAL "EXECINFO_H_DIR-NOTFOUND")

if(HAVE_EXECINFO)
  add_definitions(-DHAVE_EXECINFO)
endif(HAVE_EXECINFO)

# Select a Qt installation here, if you don't want to use system Qt
if(QT)
  # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
  set(ENV{PATH} ${QT}/bin:$ENV{PATH})
endif(QT)

# Now that we have the correct $PATH, lets find Qt!
find_package(Qt4 REQUIRED)

set(QT_DONT_USE_QTGUI 1)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDES})

# Setup OpenSSL
if(WITH_OPENSSL)
  find_package(OpenSSL)
else(WITH_OPENSSL)
  message(STATUS "Disabling OpenSSL support")
endif(WITH_OPENSSL)

if(OPENSSL_FOUND)
  if(NOT QT_DEFINITIONS MATCHES "QT_NO_OPENSSL")
    message(STATUS "Found OpenSSL support in Qt")
    add_definitions(-DHAVE_SSL)
    set(HAVE_SSL true)
    set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_SSL)
  else(NOT QT_DEFINITIONS MATCHES "QT_NO_OPENSSL")
    message(STATUS "No OpenSSL support found in Qt, disabling")
  endif(NOT QT_DEFINITIONS MATCHES "QT_NO_OPENSSL")
else(OPENSSL_FOUND)
  add_definitions(-DQT_NO_OPENSSL)
endif(OPENSSL_FOUND)

# Setup D-Bus support
if(WITH_DBUS)
  if(QT_QTDBUS_FOUND)
    message(STATUS "Found QtDBus, enabling D-Bus support")
    add_definitions(-DHAVE_DBUS)
    set(LINK_DBUS DBUS)
    set(HAVE_DBUS true)
    set(MOC_DEFINES ${MOC_DEFINES} -DHAVE_DBUS)
  else(QT_QTDBUS_FOUND)
    message(STATUS "QtDBus not found, disabling D-Bus support")
  endif(QT_QTDBUS_FOUND)
else(WITH_DBUS)
  message(STATUS "Disabling D-Bus support")
endif(WITH_DBUS)

# We need to create a version.gen
# For this, we create our genversion binary and make sure it is run every time.
add_executable(genversion ${CMAKE_SOURCE_DIR}/src/common/genversion.cpp)
target_link_libraries(genversion ${QT_LIBRARIES} ${QT_CORE_LIB_DEPENDENCIES})

get_target_property(GENVERSION_EXECUTABLE genversion LOCATION)
add_custom_target(genversion_run ALL ${GENVERSION_EXECUTABLE}
                  ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/src/common/version.gen)
add_dependencies(genversion_run genversion)

# Add needed subdirs
add_subdirectory(src/common)
include_directories(src/common)
if(WANT_CORE OR WANT_MONO)
  add_subdirectory(src/core)
  include_directories(src/core)
endif(WANT_CORE OR WANT_MONO)
if(WANT_QTCLIENT OR WANT_MONO)
  add_subdirectory(src/client)
  add_subdirectory(src/uisupport)
  add_subdirectory(src/qtui)
  include_directories(src/client)
  include_directories(src/uisupport)
  include_directories(src/qtui)
endif(WANT_QTCLIENT OR WANT_MONO)

# Make sure version.gen exists before building mod_common
add_dependencies(mod_common genversion_run)

# Generate binary translation files
include(QuasselGenerateTranslations)
quassel_generate_i18n_resource(RC_I18N ${LINGUAS})

# Add resources
qt4_add_resources(RC_ICONS src/icons/icons.qrc)
qt4_add_resources(RC_QUASSEL_ICONS src/icons/quassel-icons.qrc)
qt4_add_resources(RC_SQL src/core/sql.qrc)

# Set global buildflags
# This is very much non-portable, so don't use -DSTATICGCC until you know what
# you do.
if(STATIC AND CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-static-libgcc ${CMAKE_CXX_FLAGS}")
  link_directories(${CMAKE_BINARY_DIR}/staticlibs) # override dynamic libs
  if(HAVE_SSL)
    set(QUASSEL_SSL_LIBRARIES ssl crypto)  # these miss in static builds
  endif(HAVE_SSL)
endif(STATIC AND CMAKE_COMPILER_IS_GNUCXX)

if(STATIC AND WIN32)
  link_libraries(imm32 winmm)  # missing by default :/
   if(HAVE_SSL)
     link_libraries(${OPENSSL_LIBRARIES} libeay32MD)
   endif(HAVE_SSL)
endif(STATIC AND WIN32)

if(WIN32)
  set(WIN32_RC src/icons/win32.rc)  # for app icons on windows
endif(WIN32)

# Here comes the dirty part. Our targets need different Qt4 modules, i.e. different libs
# and defines. We can't simply include UseQt4 several times, since definitions add up.
# We workaround this by using our own macro to figure out what to add.

# This macro sets variables for additional Qt modules.
macro(setup_qt4_variables)
  set(QUASSEL_QT_LIBRARIES )
  IF(WIN32)
    set(MAIN MAIN)
  ENDIF(WIN32)
  foreach(qtmod CORE ${ARGV} ${MAIN})
    set(QUASSEL_QT_LIBRARIES ${QUASSEL_QT_LIBRARIES} ${QT_QT${qtmod}_LIBRARY} ${QT_${qtmod}_LIB_DEPENDENCIES})
  endforeach(qtmod ${ARGV})
  set(QUASSEL_QT_LIBRARIES ${QUASSEL_QT_LIBRARIES} ${QT_LIBRARIES})
endmacro(setup_qt4_variables)

# Now we have everything, so just glue the right pieces together :)
if(WANT_CORE)
  setup_qt4_variables(NETWORK SCRIPT SQL)
  add_executable(quasselcore src/common/main.cpp
                             ${RC_SQL} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quasselcore PROPERTIES 
                                    COMPILE_FLAGS "-DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_CORE")
  target_link_libraries(quasselcore mod_core mod_common 
                                    ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
endif(WANT_CORE)

if(WANT_QTCLIENT)
  setup_qt4_variables(${LINK_DBUS} GUI NETWORK)
  add_executable(quasselclient WIN32 src/common/main.cpp
                                     ${RC_ICONS} ${RC_QUASSEL_ICONS} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quasselclient PROPERTIES
                                      COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DBUILD_QTUI")
  target_link_libraries(quasselclient mod_qtui mod_uisupport mod_client mod_common
                                      ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
endif(WANT_QTCLIENT)

if(WANT_MONO)
  setup_qt4_variables(${LINK_DBUS} GUI NETWORK SCRIPT SQL)
  add_executable(quassel WIN32 src/common/main.cpp src/qtui/monoapplication.cpp
                               ${RC_ICONS} ${RC_QUASSEL_ICONS} ${RC_SQL} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quassel PROPERTIES 
                                COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_MONO")
  target_link_libraries(quassel mod_qtui mod_uisupport mod_client mod_core mod_common 
                                ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
endif(WANT_MONO)

# Build bundles for MacOSX
if(APPLE)
  add_custom_command(TARGET quasselclient POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makebundle.py
                             ${CMAKE_SOURCE_DIR} "Quassel Client" quasselclient)
  add_custom_command(TARGET quassel POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makebundle.py
                             ${CMAKE_SOURCE_DIR} "Quassel" quassel)
  if(DEPLOY)
    add_custom_command(TARGET quasselclient POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Client)
    add_custom_command(TARGET quasselcore POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Core)
  endif(DEPLOY)
endif(APPLE)

# Install rules
if(WANT_CORE)
  install(TARGETS quasselcore
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif(WANT_CORE)

if(WANT_QTCLIENT)
  install(TARGETS quasselclient
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

  install(FILES quasselclient.desktop
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif(WANT_QTCLIENT)

if(WANT_MONO)
  install(TARGETS quassel
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

  install(FILES quassel.desktop
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif(WANT_MONO)

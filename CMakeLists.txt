# This is the cmake-based build system for Quassel IRC.
# You may pass various options to cmake:
# -DWANT_(CORE|QTCLIENT|MONO)=(ON|OFF)
#                  : select binaries to build
# -DQT=/path/to/qt : Choose a Qt4 installation to use instead of the system Qt4
# -DSTATIC=ON      : Enable static building of Quassel. Use with care.
# -DSPUTDEV=ON     : Do not use.
# -DDEPLOY=ON      : Mac OS X only. Use only fore redistribution Quassel Packages!!
#
# NOTE: You need to remove CMakeCache.txt if you plan to change any of these values!

project(QuasselIRC)

# Target scopes don't work in older versions
cmake_minimum_required(VERSION 2.4.7 FATAL_ERROR)

if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 OLD)  # suppress linker warnings
endif(COMMAND cmake_policy)

# Use our own (well, KDE's) version of some modules
# In particular cmake's FindQt4 and FindOpenSSL are quite buggy
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Various options and variables that can be set on the command line
option(WANT_CORE     "Build the core (server) binary"           ON)
option(WANT_QTCLIENT "Build the Qt4 GUI client binary"          ON)
option(WANT_MONO     "Build the monolithic (all-in-one) binary" OFF)

option(STATIC        "Enable static building (might not be portable)" OFF)
option(DEPLOY        "Mac OS X only! Adds required libs to bundle resources and create a dmg. Note: requires Qt to be built with 10.4u SDK" OFF)
option(SPUTDEV       "Do not use!" OFF)

set(QT "" CACHE STRING "Path to a Qt installation to use instead of the system Qt")
set(LINGUAS "" CACHE STRING "Space-separated List of locales specifying languages that should be compiled")
 
if(STATIC)
  set(CMAKE_BUILD_TYPE Release)
endif(STATIC)

# Enable various flags on gcc
if(CMAKE_COMPILER_IS_GNUCXX)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-Wall HAVE_WALL)
  if(HAVE_WALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(HAVE_WALL)
  check_cxx_compiler_flag(-Wextra HAVE_WEXTRA)
  if(HAVE_WEXTRA)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif(HAVE_WEXTRA)
  check_cxx_compiler_flag(-ansi HAVE_ANSI)
  if(HAVE_ANSI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi")
  endif(HAVE_ANSI)
endif(CMAKE_COMPILER_IS_GNUCXX)

set(QT_MIN_VERSION "4.3.0")

if(APPLE AND DEPLOY)
  set(CMAKE_OSX_ARCHITECTURES "i386;ppc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4")
  set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
endif(APPLE AND DEPLOY)

# Enable mostly b0rked stuff (new ChatView), do not enable this unless you know what you do...
if(SPUTDEV)
  add_definitions(-DSPUTDEV)
endif(SPUTDEV)

# Set up OpenSSL
find_package(OpenSSL)
if(NOT OPENSSL_FOUND)
  add_definitions(-DQT_NO_OPENSSL)
endif(NOT OPENSSL_FOUND)

# Select a Qt installation here, if you don't want to use system Qt
if(QT)
  # FindQt4 will look for the qmake binary in $PATH, so we just prepend the Qt dir
  set(ENV{PATH} ${QT}/bin:$ENV{PATH})
endif(QT)

# Now that we have the correct $PATH, lets find Qt!
find_package(Qt4 REQUIRED)

if(QT_QTDBUS_FOUND)
  add_definitions(-DHAVE_DBUS)
  set(LINK_DBUS DBUS)
  set(HAVE_DBUS true)
endif(QT_QTDBUS_FOUND)

set(QT_DONT_USE_QTGUI 1)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDES})

# We need to create a version.gen
# For this, we create our genversion binary and make sure it is run every time.
add_executable(genversion ${CMAKE_SOURCE_DIR}/src/common/genversion.cpp)
target_link_libraries(genversion ${QT_LIBRARIES} ${QT_CORE_LIB_DEPENDENCIES})

add_custom_target(genversion_run ALL ${CMAKE_BINARY_DIR}/genversion
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
  if(OPENSSL_FOUND)
    set(QUASSEL_SSL_LIBRARIES ssl crypto)  # these miss in static builds
  endif(OPENSSL_FOUND)
endif(STATIC AND CMAKE_COMPILER_IS_GNUCXX)

if(STATIC AND WIN32)
  link_libraries(imm32 winmm)  # missing by default :/
   if(OPENSSL_FOUND)
     link_libraries(${OPENSSL_LIBRARIES} libeay32MD)
   endif(OPENSSL_FOUND)
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
  add_executable(quasselcore ${CMAKE_SOURCE_DIR}/src/common/main.cpp
                             ${RC_SQL} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quasselcore PROPERTIES 
                                    COMPILE_FLAGS "-DQT_NETWORK_LIB -DQT_SCRIPT_LIB -DQT_SQL_LIB -DBUILD_CORE")
  target_link_libraries(quasselcore mod_core mod_common 
                                    ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
endif(WANT_CORE)

if(WANT_QTCLIENT)
  setup_qt4_variables(${LINK_DBUS} GUI NETWORK)
  add_executable(quasselclient WIN32 ${CMAKE_SOURCE_DIR}/src/common/main.cpp
                                     ${RC_ICONS} ${RC_QUASSEL_ICONS} ${RC_I18N} ${WIN32_RC})
  set_target_properties(quasselclient PROPERTIES
                                      COMPILE_FLAGS "-DQT_GUI_LIB -DQT_NETWORK_LIB -DBUILD_QTUI")
  target_link_libraries(quasselclient mod_qtui mod_uisupport mod_client mod_common
                                      ${QUASSEL_QT_LIBRARIES} ${QUASSEL_SSL_LIBRARIES})
endif(WANT_QTCLIENT)

if(WANT_MONO)
  setup_qt4_variables(${LINK_DBUS} GUI NETWORK SCRIPT SQL)
  add_executable(quassel WIN32 ${CMAKE_SOURCE_DIR}/src/common/main.cpp
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

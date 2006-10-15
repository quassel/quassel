PROJECT(Quassel)

# CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2)

# Select if Quassel should be built in client, server or monolithic mode
SET(BUILD "mono" CACHE STRING "Defines which Quassel parts are to be built. Can contain 'core', 'gui' and/or 'monolithic' (which is the default).")
SET(BUILD_CORE )
SET(BUILD_GUI )
SET(BUILD_MONO )
IF(BUILD MATCHES "core")
  SET(BUILD_CORE true)
  MESSAGE(STATUS "Building Quassel core.")
ENDIF(BUILD MATCHES "core")
IF(BUILD MATCHES "gui")
  SET(BUILD_GUI true)
  MESSAGE(STATUS "Building Quassel GUI.")
ENDIF(BUILD MATCHES "gui")
IF(BUILD MATCHES "mono")
  SET(BUILD_MONO true)
  MESSAGE(STATUS "Building monolithic Quassel.")
ENDIF(BUILD MATCHES "mono")
IF(NOT BUILD_MONO AND NOT BUILD_CORE AND NOT BUILD_GUI)
  MESSAGE(FATAL_ERROR "You have not selected which parts of Quassel I should build. Aborting.\nRun 'cmake -DBUILD=<part>', where <part> contains one or more of 'core', 'gui' or 'monolithic'.")
ENDIF(NOT BUILD_MONO AND NOT BUILD_CORE AND NOT BUILD_GUI)

# Define files
SET(quassel_mono_SRCS main/main_mono.cpp)
SET(quassel_core_SRCS main/main_core.cpp)
SET(quassel_RCCS images/icons.qrc)
SET(quassel_DIRS gui core network)

# Build correct absolute paths for subdirs to include
SET(SDIRS "")
FOREACH(dir ${quassel_DIRS})
  SET(SDIRS ${SDIRS} "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
ENDFOREACH(dir)
INCLUDE_DIRECTORIES(${SDIRS})

# We need Qt4 support.
FIND_PACKAGE(Qt4 REQUIRED)

# Set needed libraries
SET(QT_USE_QTXML true)
SET(QT_DONT_USE_QTGUI true)   # This is added later if GUI is requested
INCLUDE(${QT_USE_FILE})

ADD_SUBDIRECTORY(network)
ADD_SUBDIRECTORY(core)
QT4_ADD_RESOURCES(_RCCS ${quassel_RCCS})

IF(BUILD_CORE)
  ADD_EXECUTABLE(quasselcore ${quassel_core_SRCS} ${_RCCS})
  TARGET_LINK_LIBRARIES(quasselcore core network ${QT_LIBRARIES})
ENDIF(BUILD_CORE)

IF(BUILD_GUI OR BUILD_MONO)  # OK, now we need QtGui!
  REMOVE_DEFINITIONS(-DQT_CORE_LIB -DQT_GUI_LIB ${QT_DEFINITIONS})
  SET(QT_DONT_USE_QTGUI "")
  SET(QT_INCLUDE_DIR "")
  SET(QT_LIBRARIES "")
  INCLUDE(${QT_USE_FILE})

  IF(BUILD_MONO)
    ADD_SUBDIRECTORY(gui)
    ADD_EXECUTABLE(quassel ${quassel_mono_SRCS} ${_RCCS})
    TARGET_LINK_LIBRARIES(quassel gui core network ${QT_LIBRARIES})
  ENDIF(BUILD_MONO)

  IF(BUILD_GUI)
    ADD_SUBDIRECTORY(gui)
    MESSAGE(FATAL_ERROR "Client mode not yet supported.")
  ENDIF(BUILD_GUI)

ENDIF(BUILD_GUI OR BUILD_MONO)

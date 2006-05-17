PROJECT(Quassel)

# Select if Quassel should be built in client, server or monolithic mode
SET(BUILD "mono" CACHE STRING "Defines which Quassel parts are to be built. Can contain 'server', 'client' and/or 'monolithic' (which is the default).")
SET(BUILD_SERVER )
SET(BUILD_CLIENT )
SET(BUILD_MONO )
IF(BUILD MATCHES "server")
  SET(BUILD_SERVER true)
  MESSAGE(STATUS "Building Quassel server.")
ENDIF(BUILD MATCHES "server")
IF(BUILD MATCHES "client")
  SET(BUILD_CLIENT true)
  MESSAGE(STATUS "Building Quassel client.")
ENDIF(BUILD MATCHES "client")
IF(BUILD MATCHES "mono")
  SET(BUILD_MONO true)
  MESSAGE(STATUS "Building monolithic Quassel.")
ENDIF(BUILD MATCHES "mono")
IF(NOT BUILD_MONO AND NOT BUILD_SERVER AND NOT BUILD_CLIENT)
  MESSAGE(FATAL_ERROR "You have not selected which parts of Quassel I should build. Aborting.\nRun 'cmake -DBUILD=<part>', where <part> contains one or more of 'server', 'client' or 'monolithic'.")
ENDIF(NOT BUILD_MONO AND NOT BUILD_SERVER AND NOT BUILD_CLIENT)

# We need Qt4 support.
FIND_PACKAGE(Qt4 REQUIRED)

# Set needed libraries
SET(QT_USE_QTXML true)
IF(NOT BUILD_CLIENT AND NOT BUILD_MONO)
  SET(QT_DONT_USE_QTGUI true)
  MESSAGE(STATUS "Disabling GUI support.")
ENDIF(NOT BUILD_CLIENT AND NOT BUILD_MONO)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

# Define files
SET(quassel_SRCS main/main.cpp)
SET(quassel_RCCS images/icons.qrc)
SET(quassel_DIRS gui main)

# Build correct absolute paths for subdirs to include
SET(SDIRS "")
FOREACH(dir ${quassel_DIRS})
  SET(SDIRS ${SDIRS} "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
ENDFOREACH(dir)
INCLUDE_DIRECTORIES(${SDIRS} ${QT_INCLUDES})

ADD_SUBDIRECTORY(gui)
ADD_SUBDIRECTORY(main)

QT4_ADD_RESOURCES(_RCCS ${quassel_RCCS})

ADD_EXECUTABLE(quasselbin ${quassel_SRCS} ${_RCCS})
TARGET_LINK_LIBRARIES(quasselbin main gui ${QT_LIBRARIES})

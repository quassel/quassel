PROJECT(Quassel)

# 2.4.2 had a bug with out-of-source builds and UIC dependencies.
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.3 FATAL_ERROR)

# Select if Quassel should be built in client, server or monolithic mode
SET(BUILD "mono" CACHE STRING "Defines which Quassel parts are to be built. Can contain 'core', 'gui' and/or 'monolithic' (which is the default), or 'all' to build everything.")
SET(BUILD_CORE )
SET(BUILD_QTGUI )
SET(BUILD_MONO )
IF(BUILD MATCHES "core" OR BUILD MATCHES "all")
  SET(BUILD_CORE true)
  MESSAGE("Building Quassel core.")
ENDIF(BUILD MATCHES "core" OR BUILD MATCHES "all")
IF(BUILD MATCHES "gui" OR BUILD MATCHES "all")
  SET(BUILD_QTGUI true)
  MESSAGE("Building Quassel GUI for Qt.")
ENDIF(BUILD MATCHES "gui" OR BUILD MATCHES "all")
IF(BUILD MATCHES "mono" OR BUILD MATCHES "all")
  SET(BUILD_MONO true)
  MESSAGE("Building monolithic Quassel.")
ENDIF(BUILD MATCHES "mono" OR BUILD MATCHES "all")
IF(NOT BUILD_MONO AND NOT BUILD_CORE AND NOT BUILD_QTGUI)
  MESSAGE(FATAL_ERROR "\nYou have not selected which parts of Quassel I should build. Aborting.\nRun 'cmake <path> -DBUILD=<part>', where <part> contains one or more of 'core', 'gui' or 'monolithic', or 'all' to build everything.\n")
ENDIF(NOT BUILD_MONO AND NOT BUILD_CORE AND NOT BUILD_QTGUI)

#IF(BUILD_CORE OR BUILD_QTGUI)
#  MESSAGE(FATAL_ERROR "\nBuilding of standalone core or GUI not supported at this time. Please check back later.\n")
#ENDIF(BUILD_CORE OR BUILD_QTGUI)

SET(CMAKE_BUILD_TYPE Debug)
SET(GCC_WARN "-Wall -Wextra -ansi -Wno-unused-parameter")  # may not be portable!

# Define files
SET(quassel_mono_SRCS common/build_mono.cpp)
SET(quassel_core_SRCS common/build_core.cpp)
SET(quassel_qtgui_SRCS common/build_qtgui.cpp)
SET(quassel_RCCS images/icons.qrc)
SET(quassel_DIRS common client core qtgui)

# Build correct absolute paths for subdirs to include
SET(SDIRS "")
FOREACH(dir ${quassel_DIRS})
  SET(SDIRS ${SDIRS} "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
ENDFOREACH(dir)
INCLUDE_DIRECTORIES(${SDIRS} plugins contrib/qxt)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/qtgui) # so that ui files are found

# We need Qt4 support.
SET(QT_MIN_VERSION "4.3.0")  # 4.3 is required for SSL, crypto and some other stuff
FIND_PACKAGE(Qt4 REQUIRED)

# Set needed libraries
SET(QT_USE_QTXML true)
SET(QT_USE_QTSQL true)
SET(QT_USE_QTNETWORK true)
SET(QT_USE_QTUITOOLS true)
SET(QT_DONT_USE_QTGUI true)   # This is added later if GUI is requested
INCLUDE(${QT_USE_FILE})

ADD_DEFINITIONS(${GCC_WARN})

# Define subdirs. CMake complains if a directory is added twice, so make sure this
# does not happen in any combination of the requested targets.

ADD_SUBDIRECTORY(contrib/qxt)
ADD_SUBDIRECTORY(common)
IF(BUILD_CORE)
  ADD_SUBDIRECTORY(core)
ENDIF(BUILD_CORE)
IF(BUILD_MONO AND NOT BUILD_CORE)
  ADD_SUBDIRECTORY(core)
ENDIF(BUILD_MONO AND NOT BUILD_CORE)

QT4_ADD_RESOURCES(_RCCS ${quassel_RCCS})

SET(TARGET_LIST )

IF(BUILD_CORE)
  ADD_EXECUTABLE(quasselcore ${quassel_core_SRCS} ${_RCCS})
  TARGET_LINK_LIBRARIES(quasselcore common core ${QT_LIBRARIES})
  SET(TARGET_LIST ${TARGET_LIST} quasselcore)
ENDIF(BUILD_CORE)

IF(BUILD_QTGUI OR BUILD_MONO)  # OK, now we need QtGui!
  REMOVE_DEFINITIONS(-DQT_CORE_LIB -DQT_GUI_LIB ${QT_DEFINITIONS})
  SET(QT_DONT_USE_QTGUI "")
  SET(QT_INCLUDE_DIR "")
  SET(QT_LIBRARIES "")
  INCLUDE(${QT_USE_FILE})
  ADD_SUBDIRECTORY(client)
  ADD_SUBDIRECTORY(qtgui)

  IF(BUILD_MONO)
    ADD_EXECUTABLE(quassel ${quassel_mono_SRCS} ${_RCCS})
    TARGET_LINK_LIBRARIES(quassel common client core qtgui ${QT_LIBRARIES})
    SET(TARGET_LIST ${TARGET_LIST} quassel)
  ENDIF(BUILD_MONO)

  IF(BUILD_QTGUI)
    ADD_EXECUTABLE(quasselclient ${quassel_qtgui_SRCS} ${_RCCS})
    TARGET_LINK_LIBRARIES(quasselclient common client qtgui ${QT_LIBRARIES})
    SET(TARGET_LIST ${TARGET_LIST} quasselclient)
  ENDIF(BUILD_QTGUI)

ENDIF(BUILD_QTGUI OR BUILD_MONO)

INSTALL(TARGETS ${TARGET_LIST}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

PROJECT(Quassel)

# 2.4.2 had a bug with out-of-source builds and UIC dependencies.
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.3 FATAL_ERROR)

# Select if Quassel should be built in client, server or monolithic mode
SET(BUILD "mono" CACHE STRING "Defines which Quassel parts are to be built. Can contain 'core', 'gui' and/or 'monolithic' (which is the default), or 'all' to build everything.")
SET(BUILD_CORE )
SET(BUILD_GUI )
SET(BUILD_MONO )
IF(BUILD MATCHES "core" OR BUILD MATCHES "all")
  SET(BUILD_CORE true)
  MESSAGE("Building Quassel core.")
ENDIF(BUILD MATCHES "core" OR BUILD MATCHES "all")
IF(BUILD MATCHES "gui" OR BUILD MATCHES "all")
  SET(BUILD_GUI true)
  MESSAGE("Building Quassel GUI.")
ENDIF(BUILD MATCHES "gui" OR BUILD MATCHES "all")
IF(BUILD MATCHES "mono" OR BUILD MATCHES "all")
  SET(BUILD_MONO true)
  MESSAGE("Building monolithic Quassel.")
ENDIF(BUILD MATCHES "mono" OR BUILD MATCHES "all")
IF(NOT BUILD_MONO AND NOT BUILD_CORE AND NOT BUILD_GUI)
  MESSAGE(FATAL_ERROR "\nYou have not selected which parts of Quassel I should build. Aborting.\nRun 'cmake <path> -DBUILD=<part>', where <part> contains one or more of 'core', 'gui' or 'monolithic', or 'all' to build everything.\n")
ENDIF(NOT BUILD_MONO AND NOT BUILD_CORE AND NOT BUILD_GUI)

SET(CMAKE_BUILD_TYPE Debug)

# Define files
SET(quassel_mono_SRCS main/main_mono.cpp)
SET(quassel_core_SRCS main/main_core.cpp)
SET(quassel_gui_SRCS  main/main_gui.cpp ${common_SRCS})
SET(quassel_RCCS images/icons.qrc)
SET(quassel_DIRS main gui core)

# Build correct absolute paths for subdirs to include
SET(SDIRS "")
FOREACH(dir ${quassel_DIRS})
  SET(SDIRS ${SDIRS} "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
ENDFOREACH(dir)
INCLUDE_DIRECTORIES(${SDIRS} plugins)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# We need Qt4 support.
SET(QT_MIN_VERSION "4.3.0")  # 4.3 is required for SSL, crypto and some other stuff
FIND_PACKAGE(Qt4 REQUIRED)

# Set needed libraries
SET(QT_USE_QTXML true)
SET(QT_USE_QTSQL true)
SET(QT_USE_QTNETWORK true)
SET(QT_DONT_USE_QTGUI true)   # This is added later if GUI is requested
INCLUDE(${QT_USE_FILE})

# Define subdirs. CMake complains if a directory is added twice, so make sure this
# does not happen in any combination of the requested targets.

ADD_SUBDIRECTORY(main)
IF(BUILD_CORE)
  ADD_SUBDIRECTORY(core)
ENDIF(BUILD_CORE)
IF(BUILD_MONO AND NOT BUILD_CORE)
  ADD_SUBDIRECTORY(core)
ENDIF(BUILD_MONO AND NOT BUILD_CORE)

QT4_ADD_RESOURCES(_RCCS ${quassel_RCCS})

SET(TARGET_LIST )

IF(BUILD_CORE)
  ADD_DEFINITIONS(-DBUILD_CORE)
  ADD_EXECUTABLE(quasselcore ${quassel_core_SRCS} ${_RCCS})
  TARGET_LINK_LIBRARIES(quasselcore core main ${QT_LIBRARIES})
  SET(TARGET_LIST ${TARGET_LIST} quasselcore)
ENDIF(BUILD_CORE)

IF(BUILD_GUI OR BUILD_MONO)  # OK, now we need QtGui!
  REMOVE_DEFINITIONS(-DQT_CORE_LIB -DQT_GUI_LIB ${QT_DEFINITIONS})
  SET(QT_DONT_USE_QTGUI "")
  SET(QT_INCLUDE_DIR "")
  SET(QT_LIBRARIES "")
  INCLUDE(${QT_USE_FILE})
  ADD_SUBDIRECTORY(gui)

  IF(BUILD_MONO)
    ADD_DEFINITIONS(-DBUILD_MONO)
    ADD_EXECUTABLE(quassel ${quassel_mono_SRCS} ${_RCCS})
    TARGET_LINK_LIBRARIES(quassel gui core main ${QT_LIBRARIES})
    SET(TARGET_LIST ${TARGET_LIST} quassel)
  ENDIF(BUILD_MONO)

  IF(BUILD_GUI)
    ADD_DEFINITIONS(-DBUILD_GUI)
    ADD_EXECUTABLE(quasselgui ${quassel_gui_SRCS} ${_RCCS})
    TARGET_LINK_LIBRARIES(quasselgui gui main ${QT_LIBRARIES})
    SET(TARGET_LIST ${TARGET_LIST} quasselgui)
  ENDIF(BUILD_GUI)

ENDIF(BUILD_GUI OR BUILD_MONO)

INSTALL(TARGETS ${TARGET_LIST}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

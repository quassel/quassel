# Builds the common module

set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    aliasmanager.cpp
    backlogmanager.cpp
    basichandler.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    cliparser.cpp
    identity.cpp
    ignorelistmanager.cpp
    ircchannel.cpp
    irclisthelper.cpp
    ircuser.cpp
    logger.cpp
    message.cpp
    network.cpp
    networkconfig.cpp
    quassel.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    util.cpp)

if(CMAKE_HOST_WIN32)
    set(SOURCES ${SOURCES} logbacktrace_win.cpp)
endif(CMAKE_HOST_WIN32)
if(CMAKE_HOST_UNIX)
    set(SOURCES ${SOURCES} logbacktrace_unix.cpp)
endif(CMAKE_HOST_UNIX)

set(MOC_HDRS
    aliasmanager.h
    backlogmanager.h
    basichandler.h
    buffersyncer.h
    bufferviewconfig.h
    bufferviewmanager.h
    coreinfo.h
    identity.h
    ignorelistmanager.h
    ircchannel.h
    irclisthelper.h
    ircuser.h
    network.h
    networkconfig.h
    settings.h
    signalproxy.h
    syncableobject.h)

set(HEADERS ${MOC_HDRS}
    abstractcliparser.h
    bufferinfo.h
    cliparser.h
    logger.h
    message.h
    types.h
    util.h)

if(APPLE)
  set(SOURCES ${SOURCES} mac_utils.cpp)
  set(HEADERS ${HEADERS} mac_utils.h)
endif(APPLE)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})  # for version.inc and version.gen
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES version.gen)

add_library(mod_common STATIC ${SOURCES} ${MOC})

if(APPLE)
  target_link_libraries(mod_common "-framework CoreServices" "-framework CoreFoundation")
endif(APPLE)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  find_library(libdl dl)
  if(NOT libdl MATCHES "NOTFOUND")
    target_link_libraries(mod_common ${libdl})
  endif(NOT libdl MATCHES "NOTFOUND")
endif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")

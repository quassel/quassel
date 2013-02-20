# Builds the common module

setup_qt_variables(Core Network)

set(SOURCES
    aliasmanager.cpp
    backlogmanager.cpp
    basichandler.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    cliparser.cpp
    coreinfo.cpp
    ctcpevent.cpp
    event.cpp
    eventmanager.cpp
    identity.cpp
    ignorelistmanager.cpp
    internalpeer.cpp
    ircchannel.cpp
    ircevent.cpp
    irclisthelper.cpp
    ircuser.cpp
    logger.cpp
    message.cpp
    messageevent.cpp
    network.cpp
    networkconfig.cpp
    networkevent.cpp
    peer.cpp
    quassel.cpp
    remotepeer.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    util.cpp

    protocols/legacy/legacypeer.cpp
)

set(MOC_HDRS
    aliasmanager.h
    backlogmanager.h
    basichandler.h
    buffersyncer.h
    bufferviewconfig.h
    bufferviewmanager.h
    coreinfo.h
    eventmanager.h
    identity.h
    ignorelistmanager.h
    internalpeer.h
    ircchannel.h
    irclisthelper.h
    ircuser.h
    network.h
    networkconfig.h
    peer.h
    remotepeer.h
    settings.h
    signalproxy.h
    syncableobject.h

    protocols/legacy/legacypeer.h
)

set(HEADERS ${MOC_HDRS}
    abstractcliparser.h
    bufferinfo.h
    cliparser.h
    ctcpevent.h
    event.h
    ircevent.h
    messageevent.h
    networkevent.h
    logger.h
    message.h
    protocol.h
    types.h
    util.h)

if (HAVE_QCA2)
    set(SOURCES ${SOURCES} keyevent.cpp)
    set(HEADERS ${HEADERS} keyevent.h)
endif(HAVE_QCA2)

if(APPLE)
  set(SOURCES ${SOURCES} mac_utils.cpp)
  set(HEADERS ${HEADERS} mac_utils.h)
endif(APPLE)

if(CMAKE_HOST_WIN32)
    set(SOURCES ${SOURCES} logbacktrace_win.cpp)
endif(CMAKE_HOST_WIN32)
if(CMAKE_HOST_UNIX)
    set(SOURCES ${SOURCES} logbacktrace_unix.cpp)
endif(CMAKE_HOST_UNIX)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${QUASSEL_QT_INCLUDES})  # for version.inc and version.gen

if(NOT WITH_QT5)
  qt4_wrap_cpp(MOC ${MOC_HDRS})
endif(NOT WITH_QT5)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES version.gen)

add_library(mod_common STATIC ${SOURCES} ${HEADERS} ${MOC})
set_target_properties(mod_common PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

if(APPLE)
  target_link_libraries(mod_common "-framework CoreServices" "-framework CoreFoundation")
endif(APPLE)

target_link_libraries(mod_common ${CMAKE_DL_LIBS})

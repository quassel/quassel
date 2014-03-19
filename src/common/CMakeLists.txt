# Builds the common module

set(SOURCES
    aliasmanager.cpp
    authhandler.cpp
    backlogmanager.cpp
    basichandler.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    cliparser.cpp
    compressor.cpp
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
    peerfactory.cpp
    quassel.cpp
    remotepeer.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    transfer.cpp
    transfermanager.cpp
    util.cpp

    protocols/datastream/datastreampeer.cpp
    protocols/legacy/legacypeer.cpp

    # needed for automoc
    coreinfo.h
)


if (HAVE_QCA2)
    set(SOURCES ${SOURCES} keyevent.cpp)
endif(HAVE_QCA2)

if(NOT HAVE_ZLIB)
    set(SOURCES ${SOURCES} ../../3rdparty/miniz/miniz.c)
endif(NOT HAVE_ZLIB)

if(APPLE)
  set(SOURCES ${SOURCES} mac_utils.cpp)
endif(APPLE)

if(CMAKE_HOST_WIN32)
    set(SOURCES ${SOURCES} logbacktrace_win.cpp)
endif(CMAKE_HOST_WIN32)
if(CMAKE_HOST_UNIX)
    set(SOURCES ${SOURCES} logbacktrace_unix.cpp)
endif(CMAKE_HOST_UNIX)

add_library(mod_common STATIC ${SOURCES})
qt_use_modules(mod_common Core Network)

if(APPLE)
  target_link_libraries(mod_common "-framework CoreServices" "-framework CoreFoundation")
endif(APPLE)

target_link_libraries(mod_common ${CMAKE_DL_LIBS})

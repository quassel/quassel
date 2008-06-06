# Builds the common module

set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    backlogmanager.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    global.cpp
    identity.cpp
    logger.cpp
    message.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    util.cpp
    network.cpp
    ircuser.cpp
    ircchannel.cpp)

set(HEADERS
    backlogmanager.h
    buffersyncer.h
    bufferviewconfig.h
    bufferviewmanager.h
    identity.h
    ircchannel.h
    ircuser.h
    logger.h
    network.h
    signalproxy.h
    syncableobject.h)

qt4_wrap_cpp(MOC ${HEADERS})

add_library(mod_common STATIC ${SOURCES} ${MOC})

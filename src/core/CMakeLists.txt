# Builds the core module

set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTSCRIPT 1)
include(${QT_USE_FILE})

set(SOURCES
    abstractsqlstorage.cpp
    basichandler.cpp
    core.cpp
    corebacklogmanager.cpp
    corebufferviewconfig.cpp
    corebufferviewmanager.cpp
    coresession.cpp
    coresettings.cpp
    coreusersettings.cpp
    ctcphandler.cpp
    ircserverhandler.cpp
    networkconnection.cpp
    sessionthread.cpp
    sqlitestorage.cpp
    storage.cpp
    userinputhandler.cpp)

set(HEADERS
    abstractsqlstorage.h
    basichandler.h
    core.h
    corebacklogmanager.h
    corebufferviewconfig.h
    corebufferviewmanager.h
    coresession.h
    ctcphandler.h
    ircserverhandler.h
    networkconnection.h
    sqlitestorage.h
    storage.h
    sessionthread.h
    userinputhandler.h)

if(NOT ${QT_DEFINITIONS} MATCHES "-DQT_NO_OPENSSL")
  set(SOURCES ${SOURCES} sslserver.cpp)
  set(HEADERS ${HEADERS} sslserver.h)
endif(NOT ${QT_DEFINITIONS} MATCHES "-DQT_NO_OPENSSL")

QT4_WRAP_CPP(MOC ${HEADERS})

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_core STATIC ${SOURCES} ${MOC})
add_dependencies(mod_core mod_common)

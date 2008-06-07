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

# QT_DEFINITIONS actually does not work, stuff gets included always.
# Funny enough that does not seem to be harmful, but we should still find a way to figure out
# if we have openssl in Qt...
find_package(OpenSSL)

if(OPENSSL_FOUND AND NOT ${QT_DEFINITIONS} MATCHES "-DQT_NO_OPENSSL")
  set(SOURCES ${SOURCES} sslserver.cpp)
  set(HEADERS ${HEADERS} sslserver.h)
endif(OPENSSL_FOUND AND NOT ${QT_DEFINITIONS} MATCHES "-DQT_NO_OPENSSL")

QT4_WRAP_CPP(MOC ${HEADERS})

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_core STATIC ${SOURCES} ${MOC})
add_dependencies(mod_core mod_common)

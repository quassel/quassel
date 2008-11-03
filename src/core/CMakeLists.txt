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
    corealiasmanager.cpp
    coreapplication.cpp
    corebacklogmanager.cpp
    corebufferviewconfig.cpp
    corebufferviewmanager.cpp
    corecoreinfo.cpp
    coreircchannel.cpp
    coreirclisthelper.cpp
    corenetwork.cpp
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

set(MOC_HDRS
    abstractsqlstorage.h
    basichandler.h
    core.h
    corealiasmanager.h
    coreapplication.h
    corebacklogmanager.h
    corebufferviewconfig.h
    corebufferviewmanager.h
    corecoreinfo.h
    coreircchannel.h
    coreirclisthelper.h
    corenetwork.h
    coresession.h
    ctcphandler.h
    ircserverhandler.h
    networkconnection.h
    sqlitestorage.h
    storage.h
    sessionthread.h
    userinputhandler.h)

set(HEADERS
    coresettings.h
    coreusersettings.h)

if(HAVE_SSL)
  set(SOURCES ${SOURCES} sslserver.cpp)
  set(MOC_HDRS ${MOC_HDRS} sslserver.h)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif(HAVE_SSL)

qt4_wrap_cpp(MOC ${MOC_HDRS})
qt4_add_resources(RC_SQL sql.qrc)

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_core STATIC ${SOURCES} ${MOC} ${HEADERS} ${RC_SQL})
add_dependencies(mod_core mod_common)

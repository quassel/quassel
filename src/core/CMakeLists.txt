# Builds the core module

setup_qt_variables(Network Sql Script)

set(SOURCES
    abstractsqlstorage.cpp
    core.cpp
    corealiasmanager.cpp
    coreapplication.cpp
    corebacklogmanager.cpp
    corebasichandler.cpp
    corebuffersyncer.cpp
    corebufferviewconfig.cpp
    corebufferviewmanager.cpp
    corecoreinfo.cpp
    coreidentity.cpp
    coreignorelistmanager.cpp
    coreircchannel.cpp
    coreirclisthelper.cpp
    coreircuser.cpp
    corenetwork.cpp
    corenetworkconfig.cpp
    coresession.cpp
    coresessioneventprocessor.cpp
    coresettings.cpp
    coreuserinputhandler.cpp
    coreusersettings.cpp
    ctcpparser.cpp
    eventstringifier.cpp
    ircparser.cpp
    netsplit.cpp
    oidentdconfiggenerator.cpp
    postgresqlstorage.cpp
    sessionthread.cpp
    sqlitestorage.cpp
    storage.cpp)

set(MOC_HDRS
    abstractsqlstorage.h
    core.h
    corealiasmanager.h
    coreapplication.h
    corebacklogmanager.h
    corebasichandler.h
    corebuffersyncer.h
    corebufferviewconfig.h
    corebufferviewmanager.h
    corecoreinfo.h
    coreeventmanager.h
    coreidentity.h
    coreignorelistmanager.h
    coreircchannel.h
    coreirclisthelper.h
    coreircuser.h
    corenetwork.h
    corenetworkconfig.h
    coresession.h
    coresessioneventprocessor.h
    coreuserinputhandler.h
    ctcpparser.h
    eventstringifier.h
    ircparser.h
    netsplit.h
    oidentdconfiggenerator.h
    postgresqlstorage.h
    sqlitestorage.h
    storage.h
    sessionthread.h)

set(HEADERS
    coresettings.h
    coreusersettings.h)

if(HAVE_SSL)
  set(SOURCES ${SOURCES} sslserver.cpp)
  set(MOC_HDRS ${MOC_HDRS} sslserver.h)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif(HAVE_SSL)

if(HAVE_QCA2)
  set(SOURCES ${SOURCES} cipher.cpp)
  set(HEADERS ${HEADERS} cipher.h)
  include_directories(${QCA2_INCLUDE_DIR})
endif(HAVE_QCA2)

include_directories(${CMAKE_SOURCE_DIR}/src/common ${QUASSEL_QT_INCLUDES})

if(NOT WITH_QT5)
  qt4_wrap_cpp(MOC ${MOC_HDRS})
endif(NOT WITH_QT5)

set(CORE_RCS ${CORE_RCS} core/sql.qrc PARENT_SCOPE)

add_library(mod_core STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_core mod_common)
set_target_properties(mod_core PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

if(HAVE_QCA2)
  target_link_libraries(mod_core ${QCA2_LIBRARIES})
endif(HAVE_QCA2)

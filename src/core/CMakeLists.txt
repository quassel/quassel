# Builds the core module

set(SOURCES
    abstractsqlstorage.cpp
    authenticator.cpp
    core.cpp
    corealiasmanager.cpp
    coreapplication.cpp
    coreauthhandler.cpp
    corebacklogmanager.cpp
    corebasichandler.cpp
    corebuffersyncer.cpp
    corebufferviewconfig.cpp
    corebufferviewmanager.cpp
    coredccconfig.cpp
    corehighlightrulemanager.cpp
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
    coretransfer.cpp
    coretransfermanager.cpp
    coreuserinputhandler.cpp
    coreusersettings.cpp
    ctcpparser.cpp
    eventstringifier.cpp
    ircparser.cpp
    netsplit.cpp
    oidentdconfiggenerator.cpp
    postgresqlstorage.cpp
    sessionthread.cpp
    sqlauthenticator.cpp
    sqlitestorage.cpp
    storage.cpp

    # needed for automoc
    coreeventmanager.h
)

set(LIBS )

if(HAVE_SSL)
  set(SOURCES ${SOURCES} sslserver.cpp)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (QCA2_FOUND)
    add_definitions(-DHAVE_QCA2)
    include_directories(${QCA2_INCLUDE_DIR})
    list(APPEND SOURCES cipher.cpp)
    list(APPEND LIBS ${QCA2_LIBRARIES})
endif()

if (QCA2-QT5_FOUND)
    add_definitions(-DHAVE_QCA2)
    include_directories(${QCA2-QT5_INCLUDE_DIR})
    list(APPEND SOURCES cipher.cpp)
    list(APPEND LIBS ${QCA2-QT5_LIBRARIES})
endif()

# Build with LDAP if told to do so.
if(HAVE_LDAP)
    include_directories(${LDAP_INCLUDE_DIR})
    set(SOURCES ${SOURCES} ldapauthenticator.cpp)
    set(MOC_HDRS ${MOC_HDRS} ldapauthenticator.h)
endif(HAVE_LDAP)

include_directories(${CMAKE_SOURCE_DIR}/src/common)

set(CORE_RCS ${CORE_RCS} ${CMAKE_CURRENT_SOURCE_DIR}/sql.qrc)
qt_add_resources(SOURCES ${CORE_RCS})

add_library(mod_core STATIC ${SOURCES})
qt_use_modules(mod_core Core Network Script Sql)

target_link_libraries(mod_core mod_common ${LIBS})

if(HAVE_LDAP)
    target_link_libraries(mod_core ${LDAP_LIBRARIES})
endif(HAVE_LDAP)

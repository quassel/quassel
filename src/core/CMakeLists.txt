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
    identserver.cpp
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

# Build with LDAP if told to do so.
if(HAVE_LDAP)
    include_directories(${LDAP_INCLUDE_DIR})
    set(SOURCES ${SOURCES} ldapauthenticator.cpp)
    set(MOC_HDRS ${MOC_HDRS} ldapauthenticator.h)
endif(HAVE_LDAP)

include_directories(${CMAKE_SOURCE_DIR}/src/common)

set(CORE_RCS ${CORE_RCS} ${CMAKE_CURRENT_SOURCE_DIR}/sql.qrc)
qt5_add_resources(SOURCES ${CORE_RCS})

add_library(mod_core STATIC ${SOURCES})
qt5_use_modules(mod_core Core Network Script Sql)

target_link_libraries(mod_core mod_common ${LIBS})

if (Qca-qt5_FOUND)
    target_sources(mod_core PRIVATE cipher.cpp keyevent.cpp)
    target_link_libraries(mod_core qca-qt5)
    target_compile_definitions(mod_core PUBLIC -DHAVE_QCA2)
endif()

if(HAVE_LDAP)
    target_link_libraries(mod_core ${LDAP_LIBRARIES})
endif(HAVE_LDAP)

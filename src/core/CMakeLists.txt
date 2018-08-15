quassel_add_module(Core)

target_sources(${TARGET} PRIVATE
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

if (HAVE_SSL)
    target_sources(${TARGET} PRIVATE sslserver.cpp)
endif()

if (HAVE_UMASK)
    set_property(SOURCE oidentdconfiggenerator.cpp APPEND PROPERTY COMPILE_DEFINITIONS HAVE_UMASK)
endif()

if (Ldap_FOUND)
    target_sources(${TARGET} PRIVATE ldapauthenticator.cpp)
    target_link_libraries(${TARGET} PRIVATE Ldap::Ldap)
    set_property(SOURCE core.cpp APPEND PROPERTY COMPILE_DEFINITIONS HAVE_LDAP)
endif()

if (Qca-qt5_FOUND)
    target_sources(${TARGET} PRIVATE cipher.cpp keyevent.cpp)
    target_link_libraries(${TARGET} PUBLIC qca-qt5)
    target_compile_definitions(${TARGET} PUBLIC -DHAVE_QCA2)
endif()

target_link_libraries(${TARGET}
    PUBLIC
        Qt5::Core
        Qt5::Network
        Qt5::Script
        Qt5::Sql
        Quassel::Common
)

quassel_add_resource(Sql PATTERNS SQL/*.sql)
target_link_libraries(${TARGET} PUBLIC Quassel::Resource::Sql)

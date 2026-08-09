quassel_add_module(Core EXPORT)

target_sources(${TARGET} PRIVATE
    abstractsqlstorage.cpp
    authenticator.cpp
    core.cpp
    corealiasmanager.cpp
    coreapplication.cpp
    coreauthhandler.cpp
    backgroundtaskhandler.cpp
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
    ldapescaper.cpp
    metricsserver.cpp
    netsplit.cpp
    oidentdconfiggenerator.cpp
    postgresqlstorage.cpp
    sessionthread.cpp
    sqlauthenticator.cpp
    sqlitestorage.cpp
    sslserver.cpp
    storage.cpp

    # needed for automoc
    coreeventmanager.h
)

target_link_libraries(${TARGET}
    PUBLIC
        ${QUASSEL_QT_CORE_TARGET}
        ${QUASSEL_QT_NETWORK_TARGET}
        ${QUASSEL_QT_SQL_TARGET}
        Quassel::Common
)

if (HAVE_UMASK)
    set_property(SOURCE oidentdconfiggenerator.cpp APPEND PROPERTY COMPILE_DEFINITIONS HAVE_UMASK)
endif()

if (Ldap_FOUND)
    target_sources(${TARGET} PRIVATE ldapauthenticator.cpp)
    target_link_libraries(${TARGET} PRIVATE Ldap::Ldap)
    set_property(SOURCE core.cpp APPEND PROPERTY COMPILE_DEFINITIONS HAVE_LDAP)
endif()

if (QUASSEL_HAVE_QCA)
    target_sources(${TARGET} PRIVATE cipher.cpp keyevent.cpp)
    target_link_libraries(${TARGET} PUBLIC ${QUASSEL_QCA_TARGET})
    target_compile_definitions(${TARGET} PUBLIC -DHAVE_QCA2)
endif()

quassel_add_resource(Sql PATTERNS SQL/*.sql)
target_link_libraries(${TARGET} PUBLIC Quassel::Resource::Sql)

quassel_add_module(Common EXPORT)

target_sources(${TARGET} PRIVATE
    abstractsignalwatcher.h
    aliasmanager.cpp
    authhandler.cpp
    backlogmanager.cpp
    basichandler.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    compressor.cpp
    coreinfo.cpp
    ctcpevent.cpp
    dccconfig.cpp
    event.cpp
    eventmanager.cpp
    expressionmatch.cpp
    funchelpers.h
    highlightrulemanager.cpp
    identity.cpp
    ignorelistmanager.cpp
    internalpeer.cpp
    ircchannel.cpp
    ircevent.cpp
    irclisthelper.cpp
    ircuser.cpp
    logger.cpp
    logmessage.cpp
    message.cpp
    messageevent.cpp
    network.cpp
    networkconfig.cpp
    networkevent.cpp
    nickhighlightmatcher.cpp
    peer.cpp
    peerfactory.cpp
    presetnetworks.cpp
    quassel.cpp
    remotepeer.cpp
    settings.cpp
    signalproxy.cpp
    singleton.h
    syncableobject.cpp
    transfer.cpp
    transfermanager.cpp
    types.cpp
    util.cpp

    serializers/serializers.cpp

    protocols/datastream/datastreampeer.cpp
    protocols/legacy/legacypeer.cpp

    # needed for automoc
    irccap.h
    protocol.h
)

# Needed for finding the generated version.h
target_include_directories(${TARGET} PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(${TARGET} PUBLIC
    ${CMAKE_DL_LIBS}
    Qt5::Core
    Qt5::Network
    ZLIB::ZLIB
)

if (EMBED_DATA)
    set_property(SOURCE quassel.cpp APPEND PROPERTY COMPILE_DEFINITIONS EMBED_DATA)
endif()

if (HAVE_SYSLOG)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_SYSLOG)
endif()

if (WIN32)
    target_sources(${TARGET} PRIVATE logbacktrace_win.cpp windowssignalwatcher.cpp)
else()
    if (Backtrace_FOUND)
        configure_file(backtrace_config.h.in backtrace_config.h)
        target_include_directories(${TARGET} PRIVATE ${Backtrace_INCLUDE_DIRS})
        target_link_libraries(${TARGET} PRIVATE ${Backtrace_LIBRARIES})
        set_property(SOURCE logbacktrace_unix.cpp posixsignalwatcher.cpp APPEND PROPERTY COMPILE_DEFINITIONS HAVE_BACKTRACE)
    endif()
    target_sources(${TARGET} PRIVATE logbacktrace_unix.cpp posixsignalwatcher.cpp)
endif()

if (APPLE)
    target_sources(${TARGET} PRIVATE mac_utils.cpp)
    target_link_libraries(${TARGET} PUBLIC "-framework CoreServices" "-framework CoreFoundation")
endif()

target_link_if_exists(${TARGET} PUBLIC Quassel::Resource::I18n)

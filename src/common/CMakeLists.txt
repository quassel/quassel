# Builds the common module

set(SOURCES
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
    # expressionmatchtests.cpp
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

if (HAVE_SYSLOG)
    add_definitions(-DHAVE_SYSLOG)
endif()

if (APPLE)
    set(SOURCES ${SOURCES} mac_utils.cpp)
endif()

qt5_add_resources(SOURCES ${COMMON_RCS})

add_library(mod_common STATIC ${SOURCES})
qt5_use_modules(mod_common Core Network)

if (WIN32)
    target_sources(mod_common PRIVATE logbacktrace_win.cpp windowssignalwatcher.cpp)
else()
    if (Backtrace_FOUND)
        configure_file(backtrace_config.h.in backtrace_config.h)
        target_compile_definitions(mod_common PRIVATE -DHAVE_BACKTRACE)
        target_include_directories(mod_common PRIVATE ${Backtrace_INCLUDE_DIRS})
        target_link_libraries(mod_common PRIVATE ${Backtrace_LIBRARIES})
    endif()
    target_sources(mod_common PRIVATE logbacktrace_unix.cpp posixsignalwatcher.cpp)
endif()

if (APPLE)
    target_link_libraries(mod_common "-framework CoreServices" "-framework CoreFoundation")
endif()

target_link_libraries(mod_common ${CMAKE_DL_LIBS} ZLIB::ZLIB)

# This is needed so translations are generated before trying to build the qrc.
# Should probably find a nicer solution with proper dependencies between the involved files, though...
add_dependencies(mod_common po)

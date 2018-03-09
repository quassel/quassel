# Builds the common module

set(SOURCES
    aliasmanager.cpp
    authhandler.cpp
    backlogmanager.cpp
    basichandler.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    compressor.cpp
    ctcpevent.cpp
    dccconfig.cpp
    event.cpp
    eventmanager.cpp
    highlightrulemanager.cpp
    identity.cpp
    ignorelistmanager.cpp
    internalpeer.cpp
    ircchannel.cpp
    ircevent.cpp
    irclisthelper.cpp
    ircuser.cpp
    logger.cpp
    message.cpp
    messageevent.cpp
    network.cpp
    networkconfig.cpp
    networkevent.cpp
    peer.cpp
    peerfactory.cpp
    presetnetworks.cpp
    quassel.cpp
    remotepeer.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    transfer.cpp
    transfermanager.cpp
    types.cpp
    util.cpp

    serializers/serializers.cpp

    protocols/datastream/datastreampeer.cpp
    protocols/legacy/legacypeer.cpp

    # needed for automoc
    coreinfo.h
    irccap.h
    protocol.h
)

if (USE_QT5)
    list(APPEND SOURCES qt5cliparser.cpp)
else()
    list(APPEND SOURCES cliparser.cpp)
endif()

if (QCA2_FOUND OR QCA2-QT5_FOUND)
    set(SOURCES ${SOURCES} keyevent.cpp)
endif()

if (ZLIB_FOUND)
    add_definitions(-DHAVE_ZLIB)
    include_directories(${ZLIB_INCLUDE_DIRS})
else()
    set(SOURCES ${SOURCES} ../../3rdparty/miniz/miniz.c)
endif()

if (USE_QT4)
    set(SOURCES ${SOURCES} ../../3rdparty/sha512/sha512.c)
endif()

if (HAVE_SYSLOG)
    add_definitions(-DHAVE_SYSLOG)
endif()

if (APPLE)
    set(SOURCES ${SOURCES} mac_utils.cpp)
endif()

if (WIN32)
    set(SOURCES ${SOURCES} logbacktrace_win.cpp)
else()
    if (EXECINFO_FOUND)
        add_definitions(-DHAVE_EXECINFO)
        include_directories(${EXECINFO_INCLUDES})
    endif()
    set(SOURCES ${SOURCES} logbacktrace_unix.cpp)
endif()

qt_add_resources(SOURCES ${COMMON_RCS})

add_library(mod_common STATIC ${SOURCES})
qt_use_modules(mod_common Core Network)

if (APPLE)
    target_link_libraries(mod_common "-framework CoreServices" "-framework CoreFoundation")
endif()

target_link_libraries(mod_common ${CMAKE_DL_LIBS} ${EXECINFO_LIBRARIES})

if(ZLIB_FOUND)
    target_link_libraries(mod_common ${ZLIB_LIBRARIES})
endif()

# This is needed so translations are generated before trying to build the qrc.
# Should probably find a nicer solution with proper dependencies between the involved files, though...
add_dependencies(mod_common po)

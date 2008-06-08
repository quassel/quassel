# Builds the common module

set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    backlogmanager.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    global.cpp
    identity.cpp
    logger.cpp
    message.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    util.cpp
    network.cpp
    ircuser.cpp
    ircchannel.cpp)

set(HEADERS
    backlogmanager.h
    buffersyncer.h
    bufferviewconfig.h
    bufferviewmanager.h
    identity.h
    ircchannel.h
    ircuser.h
    logger.h
    network.h
    signalproxy.h
    syncableobject.h)

qt4_wrap_cpp(MOC ${HEADERS})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})  # for version.inc and version.gen
add_definitions(-DHAVE_VERSION_GEN)  # we ensure that by deps in the main CMakeLists.txt
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES version.gen)

add_library(mod_common STATIC ${SOURCES} ${MOC})

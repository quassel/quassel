# Builds the common module

set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    aliasmanager.cpp
    backlogmanager.cpp
    bufferinfo.cpp
    buffersyncer.cpp
    bufferviewconfig.cpp
    bufferviewmanager.cpp
    cliparser.cpp
    identity.cpp
    ircchannel.cpp
    ircuser.cpp
    logger.cpp
    message.cpp
    network.cpp
    quassel.cpp
    settings.cpp
    signalproxy.cpp
    syncableobject.cpp
    util.cpp)

set(MOC_HDRS
    aliasmanager.h
    backlogmanager.h
    buffersyncer.h
    bufferviewconfig.h
    bufferviewmanager.h
    coreinfo.h
    identity.h
    ircchannel.h
    irclisthelper.h
    ircuser.h
    network.h
    settings.h
    signalproxy.h
    syncableobject.h)

set(HEADERS ${MOC_HDRS}
    bufferinfo.h
    cliparser.h
    logger.h
    message.h
    types.h
    util.h)

qt4_wrap_cpp(MOC ${MOC_HDRS})


include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})  # for version.inc and version.gen
add_definitions(-DHAVE_VERSION_GEN)  # we ensure that by deps in the main CMakeLists.txt
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES version.gen)

add_library(mod_common STATIC ${SOURCES} ${MOC} ${CMAKE_BINARY_DIR}/i18n/qrc_i18n.cxx)
set_source_files_properties(${CMAKE_BINARY_DIR}/i18n/qrc_i18n.cxx PROPERTIES GENERATED true)
add_dependencies(mod_common i18n)


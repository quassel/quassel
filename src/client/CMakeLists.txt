# Builds the client module

# We still have some minor deps to QtUi: QItemSelectionModel
# set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDBUS 1)
include(${QT_USE_FILE})

set(SOURCES
    abstractmessageprocessor.cpp
    abstractui.cpp
    backlogrequester.cpp
    buffermodel.cpp
    buffersettings.cpp
    client.cpp
    clientaliasmanager.cpp
    clientbacklogmanager.cpp
    clientbufferviewconfig.cpp
    clientbufferviewmanager.cpp
    clientidentity.cpp
    clientirclisthelper.cpp
    clientsettings.cpp
    clientsyncer.cpp
    clientuserinputhandler.cpp
    execwrapper.cpp
    irclistmodel.cpp
    messagefilter.cpp
    messagemodel.cpp
    networkmodel.cpp
    selectionmodelsynchronizer.cpp
    treemodel.cpp)

set(MOC_HDRS
    abstractmessageprocessor.h
    abstractui.h
    buffermodel.h
    client.h
    clientaliasmanager.h
    clientbacklogmanager.h
    clientbufferviewconfig.h
    clientbufferviewmanager.h
    clientcoreinfo.h
    clientidentity.h
    clientirclisthelper.h
    clientuserinputhandler.h
    clientsyncer.h
    execwrapper.h
    irclistmodel.h
    messagefilter.h
    messagemodel.h
    networkmodel.h
    selectionmodelsynchronizer.h
    treemodel.h)

set(HEADERS
    backlogsettings.h
    backlogrequester.h
    buffersettings.h
    clientsettings.h)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_client STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_client mod_common)

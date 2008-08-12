# Builds the client module

# We still have some minor deps to QtUi: QItemSelectionModel
# set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDBUS 1)
include(${QT_USE_FILE})

set(SOURCES
    abstractmessageprocessor.cpp
    buffer.cpp
    buffermodel.cpp
    buffersettings.cpp
    client.cpp
    clientbacklogmanager.cpp
    clientirclisthelper.cpp
    clientsettings.cpp
    clientsyncer.cpp
    irclistmodel.cpp
    messagefilter.cpp
    messagemodel.cpp
    mappedselectionmodel.cpp
    networkmodel.cpp
    quasselui.cpp
    selectionmodelsynchronizer.cpp
    treemodel.cpp)

set(MOC_HDRS
    abstractmessageprocessor.h
    buffer.h
    buffermodel.h
    client.h
    clientbacklogmanager.h
    clientcoreinfo.h
    clientirclisthelper.h
    clientsyncer.h
    irclistmodel.h
    messagefilter.h
    messagemodel.h
    networkmodel.h
    mappedselectionmodel.h
    quasselui.h
    selectionmodelsynchronizer.h
    treemodel.h)

set(HEADERS
    buffersettings.h
    clientsettings.h)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_client STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_client mod_common)

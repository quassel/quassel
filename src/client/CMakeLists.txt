# Builds the client module

# We still have some minor deps to QtUi: QItemSelectionModel
# set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    buffer.cpp
    buffermodel.cpp
    buffersettings.cpp
    client.cpp
    clientbacklogmanager.cpp
    clientsettings.cpp
    clientsyncer.cpp
    mappedselectionmodel.cpp
    networkmodel.cpp
    selectionmodelsynchronizer.cpp
    treemodel.cpp)

set(HEADERS
    buffer.h
    buffermodel.h
    client.h
    clientbacklogmanager.h
    clientsyncer.h
    networkmodel.h
    mappedselectionmodel.h
    quasselui.h
    selectionmodelsynchronizer.h
    treemodel.h)

qt4_wrap_cpp(MOC ${HEADERS})

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_client STATIC ${SOURCES} ${MOC})
add_dependencies(mod_client mod_common)


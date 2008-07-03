# Builds the client module

# We still have some minor deps to QtUi: QItemSelectionModel
# set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDBUS 1)
include(${QT_USE_FILE})

set(SOURCES
    buffer.cpp
    buffermodel.cpp
    buffersettings.cpp
    client.cpp
    clientbacklogmanager.cpp
    clientirclisthelper.cpp
    clientsettings.cpp
    clientsyncer.cpp
    irclistmodel.cpp
    mappedselectionmodel.cpp
    networkmodel.cpp
    selectionmodelsynchronizer.cpp
    treemodel.cpp)

set(MOC_HDRS
    buffer.h
    buffermodel.h
    client.h
    clientbacklogmanager.h
    clientcoreinfo.h
    clientirclisthelper.h
    clientsyncer.h
    irclistmodel.h
    networkmodel.h
    mappedselectionmodel.h
    quasselui.h
    selectionmodelsynchronizer.h
    treemodel.h)

set(HEADERS
    buffersettings.h
    clientsettings.h)

if(SPUTDEV)
  set(SOURCES ${SOURCES} messagefilter.cpp messagemodel.cpp)
  set(MOC_HDRS ${MOC_HDRS} messagefilter.h messagemodel.h)
endif(SPUTDEV)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_SOURCE_DIR}/src/common)

add_library(mod_client STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_client mod_common)


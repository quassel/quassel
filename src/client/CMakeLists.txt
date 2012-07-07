# Builds the client module

# In Qt4, we still have some minor deps to QtUi: QItemSelectionModel, QSortFilterProxyModel
# Still in Qt5: QAbstractItemView in BufferModel

set(_modules )

if(WITH_QT5)
  list(APPEND _modules Widgets)
endif(WITH_QT5)

if(HAVE_DBUS)
  list(APPEND _modules DBus)
endif(HAVE_DBUS)

setup_qt_variables(Gui Network ${_modules})

set(SOURCES
    abstractmessageprocessor.cpp
    abstractui.cpp
    backlogrequester.cpp
    buffermodel.cpp
    buffersettings.cpp
    bufferviewoverlay.cpp
    client.cpp
    clientaliasmanager.cpp
    clientbacklogmanager.cpp
    clientbufferviewconfig.cpp
    clientbufferviewmanager.cpp
    clientcoreinfo.cpp
    clientidentity.cpp
    clientignorelistmanager.cpp
    clientirclisthelper.cpp
    clientsettings.cpp
    clientuserinputhandler.cpp
    coreaccount.cpp
    coreaccountmodel.cpp
    coreconnection.cpp
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
    bufferviewoverlay.h
    client.h
    clientaliasmanager.h
    clientbacklogmanager.h
    clientbufferviewconfig.h
    clientbufferviewmanager.h
    clientcoreinfo.h
    clientidentity.h
    clientignorelistmanager.h
    clientirclisthelper.h
    clientuserinputhandler.h
    coreaccountmodel.h
    coreconnection.h
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
    clientsettings.h
    coreaccount.h
   )

include_directories(${CMAKE_SOURCE_DIR}/src/common ${QUASSEL_QT_INCLUDES})

if(NOT WITH_QT5)
  qt4_wrap_cpp(MOC ${MOC_HDRS})
endif(NOT WITH_QT5)

add_library(mod_client STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_client mod_common)

set_target_properties(mod_client PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

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
    backlogrequester.cpp
    buffermodel.cpp
    buffersettings.cpp
    bufferviewoverlay.cpp
    client.cpp
    clientaliasmanager.cpp
    clientauthhandler.cpp
    clientbacklogmanager.cpp
    clientbufferviewconfig.cpp
    clientbufferviewmanager.cpp
    clientidentity.cpp
    clientignorelistmanager.cpp
    clientirclisthelper.cpp
    clientsettings.cpp
    clienttransfer.cpp
    clienttransfermanager.cpp
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
    treemodel.cpp

    # needed for automoc
    abstractui.h
    clientcoreinfo.h
)

include_directories(${CMAKE_SOURCE_DIR}/src/common ${QUASSEL_QT_INCLUDES})

add_library(mod_client STATIC ${SOURCES})
add_dependencies(mod_client mod_common)

set_target_properties(mod_client PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

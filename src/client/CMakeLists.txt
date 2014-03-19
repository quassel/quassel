# Builds the client module

# In Qt4, we still have some minor deps to QtGui: QItemSelectionModel, QSortFilterProxyModel
# Still in Qt5: QAbstractItemView in BufferModel

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

if (WITH_QT5)
    list(APPEND qt_modules Widgets)
endif()

if (HAVE_DBUS)
    list(APPEND qt_modules DBus)
endif()

add_library(mod_client STATIC ${SOURCES})
qt_use_modules(mod_client Network Core Gui ${qt_modules})
add_dependencies(mod_client mod_common)

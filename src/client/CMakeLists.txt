# Builds the client module

# We still have a minor dep to QtWidgets: QAbstractItemView in BufferModel

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
    transfermodel.cpp
    treemodel.cpp

    # needed for automoc
    abstractui.h
)

qt5_add_resources(SOURCES ${CLIENT_RCS})

add_library(mod_client STATIC ${SOURCES})
qt5_use_modules(mod_client Network Core Gui Widgets)

target_link_libraries(mod_client mod_common)

quassel_add_module(Client)

target_sources(${TARGET} PRIVATE
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

target_link_libraries(${TARGET}
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets  # QAbstractItemView in BufferModel
        Quassel::Common
)

target_link_if_exists(${TARGET} PUBLIC Quassel::Resource::Data)

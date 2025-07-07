quassel_add_module(Client EXPORT)

target_sources(${TARGET} PRIVATE
    abstractmessageprocessor.cpp
    backlogrequester.cpp
    backlogsettings.cpp
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
        ${QT_FULL}::Core
        ${QT_FULL}::Gui
        ${QT_FULL}::Network
        ${QT_FULL}::Widgets  # QAbstractItemView in BufferModel
        Quassel::Common
)

if (EMBED_DATA)
    set_property(SOURCE client.cpp APPEND PROPERTY COMPILE_DEFINITIONS EMBED_DATA)
endif()

target_link_if_exists(${TARGET} PUBLIC Quassel::Resource::Data)

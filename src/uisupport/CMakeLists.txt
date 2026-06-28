quassel_add_module(UiSupport EXPORT)

target_sources(${TARGET} PRIVATE
    aboutdata.cpp
    abstractbuffercontainer.cpp
    abstractitemview.cpp
    action.cpp
    actioncollection.cpp
    bufferhotlistfilter.cpp
    bufferview.cpp
    bufferviewfilter.cpp
    bufferviewoverlayfilter.cpp
    clearablelineedit.cpp
    clickable.cpp
    clickablelabel.cpp
    colorbutton.cpp
    contextmenuactionprovider.cpp
    flatproxymodel.cpp
    fontselector.cpp
    graphicalui.cpp
    icon.cpp
    multilineedit.cpp
    networkmodelcontroller.cpp
    nickview.cpp
    nickviewfilter.cpp
    qssparser.cpp
    resizingstackedwidget.cpp
    settingspage.cpp
    styledlabel.cpp
    tabcompleter.cpp
    toolbaractionprovider.cpp
    treeviewtouch.cpp
    uisettings.cpp
    uistyle.cpp
    widgethelpers.h

    # needed for automoc
    abstractnotificationbackend.h
)

target_link_libraries(${TARGET}
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Widgets
        Quassel::Client
        Quassel::Common
)

if (EMBED_DATA)
    set_property(SOURCE graphicalui.cpp APPEND PROPERTY COMPILE_DEFINITIONS EMBED_DATA)
endif()

if (WITH_KF6)
    target_link_libraries(${TARGET}
        PUBLIC
            KF6::CoreAddons
            KF6::TextWidgets
            KF6::XmlGui
    )
endif()

if (KF6Sonnet_FOUND)
    target_link_libraries(${TARGET} PRIVATE KF6::SonnetUi)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_SONNET)
endif()

target_link_if_exists(${TARGET}
    PUBLIC
        Quassel::Resource::HicolorIcons
        Quassel::Resource::Icons
        Quassel::Resource::IconThemes
        Quassel::Resource::Pics
)

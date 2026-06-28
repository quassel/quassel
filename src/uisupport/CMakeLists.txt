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
        ${QUASSEL_QT_CORE_TARGET}
        ${QUASSEL_QT_GUI_TARGET}
        ${QUASSEL_QT_NETWORK_TARGET}
        ${QUASSEL_QT_WIDGETS_TARGET}
        Quassel::Client
        Quassel::Common
)

if (EMBED_DATA)
    set_property(SOURCE graphicalui.cpp APPEND PROPERTY COMPILE_DEFINITIONS EMBED_DATA)
endif()

if (WITH_KF)
    target_link_libraries(${TARGET}
        PUBLIC
            ${QUASSEL_KF_COREADDONS_TARGET}
            ${QUASSEL_KF_TEXTWIDGETS_TARGET}
            ${QUASSEL_KF_XMLGUI_TARGET}
    )
endif()

if (QUASSEL_HAVE_KF_SONNET)
    target_link_libraries(${TARGET} PRIVATE ${QUASSEL_KF_SONNETUI_TARGET})
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_SONNET)
endif()

target_link_if_exists(${TARGET}
    PUBLIC
        Quassel::Resource::HicolorIcons
        Quassel::Resource::Icons
        Quassel::Resource::IconThemes
        Quassel::Resource::Pics
)

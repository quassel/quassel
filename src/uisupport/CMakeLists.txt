# Builds the uisupport module

set(SOURCES
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

    # needed for automoc
    abstractnotificationbackend.h
)

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
)

add_library(mod_uisupport STATIC ${SOURCES})
qt5_use_modules(mod_uisupport Core Gui Network Widgets)

target_link_libraries(mod_uisupport mod_client mod_common)

if (WITH_KF5)
    target_link_libraries(mod_uisupport KF5::CoreAddons KF5::TextWidgets KF5::XmlGui)
endif()

if (KF5Sonnet_FOUND)
    add_definitions(-DHAVE_SONNET)
    target_link_libraries(mod_uisupport KF5::SonnetUi)
endif()

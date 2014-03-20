# Builds the uisupport module

set(SOURCES
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
    iconloader.cpp
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
    uisettings.cpp
    uistyle.cpp

    # needed for automoc
    abstractnotificationbackend.h
)

if(HAVE_KDE)
    set(SOURCES ${SOURCES} kcmdlinewrapper.cpp)
endif(HAVE_KDE)

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
)

if(HAVE_QCA2)
  include_directories(${QCA2_INCLUDE_DIR})
endif(HAVE_QCA2)

if (USE_QT5)
    list(APPEND qt_modules Widgets)
endif()

add_library(mod_uisupport STATIC ${SOURCES})
qt_use_modules(mod_uisupport Core Gui Network ${qt_modules})

add_dependencies(mod_uisupport mod_common mod_client)

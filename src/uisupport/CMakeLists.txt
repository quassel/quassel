# Builds the uisupport module

set(QT_DONT_USE_QTGUI 0)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

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
   )

set(MOC_HDRS
    abstractbuffercontainer.h
    abstractitemview.h
    abstractnotificationbackend.h
    action.h
    actioncollection.h
    bufferhotlistfilter.h
    bufferview.h
    bufferviewfilter.h
    bufferviewoverlayfilter.h
    clearablelineedit.h
    clickablelabel.h
    colorbutton.h
    contextmenuactionprovider.h
    flatproxymodel.h
    fontselector.h
    graphicalui.h
    iconloader.h
    multilineedit.h
    networkmodelcontroller.h
    nickview.h
    nickviewfilter.h
    resizingstackedwidget.h
    settingspage.h
    styledlabel.h
    tabcompleter.h
    toolbaractionprovider.h
    uistyle.h
   )

set(HEADERS
    clickable.h
    icon.h
    qssparser.h
    uisettings.h
   )

if(HAVE_KDE)
    set(SOURCES ${SOURCES} kcmdlinewrapper.cpp)
    set(HEADERS ${HEADERS} kcmdlinewrapper.h)
endif(HAVE_KDE)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client)

add_library(mod_uisupport STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_uisupport mod_common mod_client)

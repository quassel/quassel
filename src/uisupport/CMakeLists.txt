# Builds the uisupport module

if(WITH_QT5)
  setup_qt_variables(Widgets Network)
else(WITH_QT5)
  setup_qt_variables(Gui Network)
endif(WITH_QT5)

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
                    ${QUASSEL_QT_INCLUDES})

if(HAVE_QCA2)
  include_directories(${QCA2_INCLUDE_DIR})
endif(HAVE_QCA2)

add_library(mod_uisupport STATIC ${SOURCES})
add_dependencies(mod_uisupport mod_common mod_client)
set_target_properties(mod_uisupport PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

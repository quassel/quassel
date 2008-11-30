# Builds the uisupport module

set(QT_DONT_USE_QTGUI 0)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    abstractbuffercontainer.cpp
    abstractitemview.cpp
    abstractnotificationbackend.cpp
    action.cpp
    actioncollection.cpp
    bufferview.cpp
    bufferviewfilter.cpp
    clearablelineedit.cpp
    clickablelabel.cpp
    colorbutton.cpp
    icon.cpp
    iconloader.cpp
    inputline.cpp
    networkmodelactionprovider.cpp
    nickview.cpp
    nickviewfilter.cpp
    settingspage.cpp
    tabcompleter.cpp
    uisettings.cpp
    uistyle.cpp)

set(MOC_HDRS
    abstractbuffercontainer.h
    abstractitemview.h
    abstractnotificationbackend.h
    action.h
    actioncollection.h
    bufferview.h
    bufferviewfilter.h
    clearablelineedit.h
    clickablelabel.h
    colorbutton.h
    iconloader.h
    inputline.h
    networkmodelactionprovider.h
    nickview.h
    nickviewfilter.h
    settingspage.h
    tabcompleter.h)

set(HEADERS
    icon.h
    uisettings.h
    uistyle.h)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client)

add_library(mod_uisupport STATIC ${SOURCES} ${MOC} ${HEADERS})
add_dependencies(mod_uisupport mod_common mod_client)

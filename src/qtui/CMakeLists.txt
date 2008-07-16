# Builds the qtui module

set(QT_DONT_USE_QTGUI 0)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

set(SOURCES
    aboutdlg.cpp
    bufferwidget.cpp
    channellistdlg.cpp
    coreconfigwizard.cpp
    coreconnectdlg.cpp
    coreinfodlg.cpp
    debugconsole.cpp
    inputwidget.cpp
    jumpkeyhandler.cpp
    mainwin.cpp
    nicklistwidget.cpp
    qtui.cpp
    qtuisettings.cpp
    qtuistyle.cpp
    settingsdlg.cpp
    settingspagedlg.cpp
    titlesetter.cpp
    topiclabel.cpp
    topicwidget.cpp
    verticaldock.cpp)

set(MOC_HDRS
    aboutdlg.h
    bufferwidget.h
    channellistdlg.h
    coreconfigwizard.h
    coreconnectdlg.h
    coreinfodlg.h
    debugconsole.h
    inputwidget.h
    jumpkeyhandler.h
    mainwin.h
    nicklistwidget.h
    qtui.h
    settingsdlg.h
    settingspagedlg.h
    titlesetter.h
    topiclabel.h
    topicwidget.h
    verticaldock.h)

set(HEADERS
    qtuisettings.h
    qtuistyle.h)

if(SPUTDEV)
  set(SOURCES ${SOURCES} chatitem.cpp chatline.cpp chatlinemodelitem.cpp
                         chatlinemodel.cpp chatscene.cpp chatview.cpp)
  set(MOC_HDRS ${MOC_HDRS} chatlinemodel.h chatscene.h chatview.h)
  set(HEADERS ${HEADERS} chatitem.h chatline.h chatlinemodelitem.h)
else(SPUTDEV)
  set(SOURCES ${SOURCES} chatline-old.cpp chatwidget.cpp)
  set(MOC_HDRS ${MOC_HDRS} chatline-old.h chatwidget.h)
endif(SPUTDEV)

set(FORMS
    aboutdlg.ui
    bufferviewwidget.ui
    bufferwidget.ui
    channellistdlg.ui
    coreaccounteditdlg.ui
    coreconfigwizardintropage.ui
    coreconfigwizardadminuserpage.ui
    coreconfigwizardstorageselectionpage.ui
    coreconfigwizardsyncpage.ui
    coreconnectdlg.ui
    coreinfodlg.ui
    debugconsole.ui
    inputwidget.ui
    mainwin.ui
    nicklistwidget.ui
    settingsdlg.ui
    settingspagedlg.ui
    topicwidget.ui)

foreach(FORM ${FORMS})
  set(FORMPATH ${FORMPATH} ui/${FORM})
endforeach(FORM ${FORMS})

# handle settingspages
include(settingspages/settingspages.inc)
foreach(SP ${SETTINGSPAGES})
  set(SPSRC ${SPSRC} settingspages/${SP}settingspage.cpp)
  set(SPHDR ${SPHDR} settingspages/${SP}settingspage.h)
  set(SPFRM ${SPFRM} settingspages/${SP}settingspage.ui)
endforeach(SP ${SETTINGSPAGES})
foreach(SRC ${SP_SOURCES})
  set(SPSRC ${SPSRC} settingspages/${SRC})
endforeach(SRC ${SP_SOURCES})
foreach(HDR ${SP_HEADERS})
  set(SPHDR ${SPHDR} settingspages/${HDR})
endforeach(HDR ${SP_HEADERS})
foreach(FRM ${SP_FORMS})
  set(SPFRM ${SPFRM} settingspages/${FRM})
endforeach(FRM ${SP_FORMS})


qt4_wrap_cpp(MOC ${MOC_HDRS} ${SPHDR})
qt4_wrap_ui(UI ${FORMPATH} ${SPFRM})

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
                    ${CMAKE_SOURCE_DIR}/src/qtui
                    ${CMAKE_SOURCE_DIR}/src/uisupport
                    ${CMAKE_CURRENT_BINARY_DIR})

add_library(mod_qtui STATIC ${SOURCES} ${SPSRC} ${MOC} ${UI} ${HEADERS})
add_dependencies(mod_qtui mod_common mod_client mod_uisupport)

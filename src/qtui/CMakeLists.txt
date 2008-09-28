# Builds the qtui module

set(QT_DONT_USE_QTGUI 0)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDBUS 1)
include(${QT_USE_FILE})

set(SOURCES
    aboutdlg.cpp
    bufferwidget.cpp
    channellistdlg.cpp
    chatitem.cpp
    chatline.cpp
    chatlinemodel.cpp
    chatlinemodelitem.cpp
    chatmonitorfilter.cpp
    chatmonitorview.cpp
    chatscene.cpp
    chatview.cpp
    chatviewsearchbar.cpp
    chatviewsearchcontroller.cpp
    chatviewsettings.cpp
    columnhandleitem.cpp
    coreconfigwizard.cpp
    coreconnectdlg.cpp
    coreinfodlg.cpp
    debugconsole.cpp
    inputwidget.cpp
    jumpkeyhandler.cpp
    mainwin.cpp
    msgprocessorstatuswidget.cpp
    nicklistwidget.cpp
    qtui.cpp
    qtuiapplication.cpp
    qtuimessageprocessor.cpp
    qtuisettings.cpp
    qtuistyle.cpp
    sessionsettings.cpp
    settingsdlg.cpp
    settingspagedlg.cpp
    titlesetter.cpp
    topiclabel.cpp
    topicwidget.cpp
    verticaldock.cpp
    webpreviewitem.cpp)

set(MOC_HDRS
    aboutdlg.h
    bufferwidget.h
    channellistdlg.h
    chatlinemodel.h
    chatmonitorfilter.h
    chatmonitorview.h
    chatscene.h
    chatview.h
    chatviewsearchbar.h
    chatviewsearchcontroller.h
    columnhandleitem.h
    coreconfigwizard.h
    coreconnectdlg.h
    coreinfodlg.h
    debugconsole.h
    inputwidget.h
    jumpkeyhandler.h
    mainwin.h
    msgprocessorstatuswidget.h
    nicklistwidget.h
    qtui.h
    qtuiapplication.h
    qtuimessageprocessor.h
    settingsdlg.h
    settingspagedlg.h
    titlesetter.h
    topiclabel.h
    topicwidget.h
    verticaldock.h)

set(HEADERS
    chatitem.h
    chatline.h
    chatlinemodelitem.h
    chatviewsettings.h
    qtuisettings.h
    qtuistyle.h
    webpreviewitem.h)

set(FORMS
    aboutdlg.ui
    bufferviewwidget.ui
    bufferwidget.ui
    channellistdlg.ui
    chatviewsearchbar.ui
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
    msgprocessorstatuswidget.ui
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

IF(HAVE_DBUS)
  qt4_add_dbus_interface(DBUS ../../interfaces/org.freedesktop.Notifications.xml desktopnotifications)
ENDIF(HAVE_DBUS)

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
                    ${CMAKE_SOURCE_DIR}/src/qtui
                    ${CMAKE_SOURCE_DIR}/src/uisupport
                    ${CMAKE_CURRENT_BINARY_DIR})

add_library(mod_qtui STATIC ${SOURCES} ${SPSRC} ${MOC} ${DBUS} ${UI} ${HEADERS})
add_dependencies(mod_qtui mod_common mod_client mod_uisupport)

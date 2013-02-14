# Builds the qtui module

set(_modules )

if(WITH_QT5)
  list(APPEND _modules Widgets)
endif(WITH_QT5)

if(HAVE_DBUS)
  list(APPEND _modules DBus)
endif(HAVE_DBUS)

if(HAVE_WEBKIT)
  list(APPEND _modules Webkit XmlPatterns)
endif(HAVE_WEBKIT)

setup_qt_variables(Gui Network ${_modules})

set(SOURCES
    aboutdlg.cpp
    awaylogfilter.cpp
    awaylogview.cpp
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
    coreconnectionstatuswidget.cpp
    coreinfodlg.cpp
    debugbufferviewoverlay.cpp
    debugconsole.cpp
    debuglogwidget.cpp
    debugmessagemodelfilter.cpp
    inputwidget.cpp
    ircconnectionwizard.cpp
    legacysystemtray.cpp
    mainpage.cpp
    mainwin.cpp
    markerlineitem.cpp
    msgprocessorstatuswidget.cpp
    nicklistwidget.cpp
    qtui.cpp
    qtuiapplication.cpp
    qtuimessageprocessor.cpp
    qtuisettings.cpp
    qtuistyle.cpp
    settingsdlg.cpp
    settingspagedlg.cpp
    simplenetworkeditor.cpp
    systemtray.cpp
    systraynotificationbackend.cpp
    taskbarnotificationbackend.cpp
    titlesetter.cpp
    topicwidget.cpp
    verticaldock.cpp
    webpreviewitem.cpp)

set(MOC_HDRS
    aboutdlg.h
    awaylogfilter.h
    awaylogview.h
    bufferwidget.h
    channellistdlg.h
    chatitem.h
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
    coreconnectionstatuswidget.h
    coreinfodlg.h
    debugbufferviewoverlay.h
    debugconsole.h
    debuglogwidget.h
    debugmessagemodelfilter.h
    inputwidget.h
    ircconnectionwizard.h
    legacysystemtray.h
    mainpage.h
    mainwin.h
    markerlineitem.h
    msgprocessorstatuswidget.h
    nicklistwidget.h
    qtui.h
    qtuiapplication.h
    qtuimessageprocessor.h
    qtuistyle.h
    settingsdlg.h
    settingspagedlg.h
    simplenetworkeditor.h
    systemtray.h
    systraynotificationbackend.h
    taskbarnotificationbackend.h
    titlesetter.h
    topicwidget.h
    verticaldock.h)

set(HEADERS
    chatline.h
    chatlinemodelitem.h
    chatviewsettings.h
    qtuisettings.h
    webpreviewitem.h)

set(FORMS
    aboutdlg.ui
    bufferviewwidget.ui
    bufferwidget.ui
    channellistdlg.ui
    chatviewsearchbar.ui
    coreconfigwizardintropage.ui
    coreconfigwizardadminuserpage.ui
    coreconfigwizardstorageselectionpage.ui
    coreconfigwizardsyncpage.ui
    coreconnectauthdlg.ui
    coreconnectionstatuswidget.ui
    coreinfodlg.ui
    debugbufferviewoverlay.ui
    debugconsole.ui
    debuglogwidget.ui
    inputwidget.ui
    msgprocessorstatuswidget.ui
    nicklistwidget.ui
    settingsdlg.ui
    settingspagedlg.ui
    simplenetworkeditor.ui
    topicwidget.ui)

if(HAVE_KDE)
  set(SOURCES ${SOURCES} knotificationbackend.cpp)
  set(MOC_HDRS ${MOC_HDRS} knotificationbackend.h)
else(HAVE_KDE)
  if(HAVE_PHONON)
    set(SOURCES ${SOURCES} phononnotificationbackend.cpp)
    set(MOC_HDRS ${MOC_HDRS} phononnotificationbackend.h)
    set(FORMS ${FORMS} phononnotificationconfigwidget.ui)
    include_directories(${PHONON_INCLUDES})
  endif(HAVE_PHONON)
endif(HAVE_KDE)

if(HAVE_DBUS)
  set(SOURCES ${SOURCES} statusnotifieritem.cpp statusnotifieritemdbus.cpp dockmanagernotificationbackend.cpp)
  set(MOC_HDRS ${MOC_HDRS} statusnotifieritem.h statusnotifieritemdbus.h dockmanagernotificationbackend.h)
  set(FORMS ${FORMS})
  if(WITH_QT5)
    qt5_add_dbus_interface(DBUS ../../interfaces/org.kde.StatusNotifierWatcher.xml statusnotifierwatcher)
    qt5_add_dbus_interface(DBUS ../../interfaces/org.freedesktop.Notifications.xml notificationsclient)
    qt5_add_dbus_adaptor(DBUS ../../interfaces/org.kde.StatusNotifierItem.xml statusnotifieritemdbus.h StatusNotifierItemDBus)
  else(WITH_QT5)
    qt4_add_dbus_interface(DBUS ../../interfaces/org.kde.StatusNotifierWatcher.xml statusnotifierwatcher)
    qt4_add_dbus_interface(DBUS ../../interfaces/org.freedesktop.Notifications.xml notificationsclient)
    qt4_add_dbus_adaptor(DBUS ../../interfaces/org.kde.StatusNotifierItem.xml statusnotifieritemdbus.h StatusNotifierItemDBus)
  endif(WITH_QT5)
endif(HAVE_DBUS)

if(HAVE_SSL)
  set(SOURCES ${SOURCES} sslinfodlg.cpp)
  set(MOC_HDRS ${MOC_HDRS} sslinfodlg.h)
  set(FORMS ${FORMS} sslinfodlg.ui)
endif(HAVE_SSL)

if(INDICATEQT_FOUND)
  set(SOURCES ${SOURCES} indicatornotificationbackend.cpp)
  set(MOC_HDRS ${MOC_HDRS} indicatornotificationbackend.h)
  set(FORMS ${FORMS} indicatornotificationconfigwidget.ui)
  include_directories(${INDICATEQT_INCLUDE_DIRS})
endif(INDICATEQT_FOUND)

if(HAVE_NOTIFICATION_CENTER)
  set(SOURCES ${SOURCES} osxnotificationbackend.mm)
  set(MOC_HDRS ${MOC_HDRS} osxnotificationbackend.h)
endif()

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

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
                    ${CMAKE_SOURCE_DIR}/src/qtui
                    ${CMAKE_SOURCE_DIR}/src/qtui/settingspages
                    ${CMAKE_SOURCE_DIR}/src/uisupport
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${QUASSEL_QT_INCLUDES})

if(WITH_QT5)
  qt5_wrap_ui(UI ${FORMPATH} ${SPFRM})
else(WITH_QT5)
  qt4_wrap_cpp(MOC ${MOC_HDRS} ${SPHDR})
  qt4_wrap_ui(UI ${FORMPATH} ${SPFRM})
endif(WITH_QT5)

add_library(mod_qtui STATIC ${SOURCES} ${SPSRC} ${MOC} ${DBUS} ${UI} ${HEADERS})
add_dependencies(mod_qtui mod_common mod_client mod_uisupport)
set_target_properties(mod_qtui PROPERTIES COMPILE_FLAGS "${QUASSEL_QT_COMPILEFLAGS}")

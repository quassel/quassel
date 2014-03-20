# Builds the qtui module

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
    receivefiledlg.cpp
    settingsdlg.cpp
    settingspagedlg.cpp
    simplenetworkeditor.cpp
    systemtray.cpp
    systraynotificationbackend.cpp
    taskbarnotificationbackend.cpp
    titlesetter.cpp
    topicwidget.cpp
    verticaldock.cpp
    webpreviewitem.cpp
)

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
    receivefiledlg.ui
    topicwidget.ui)

if(HAVE_KDE)
  set(SOURCES ${SOURCES} knotificationbackend.cpp)
else(HAVE_KDE)
  if(HAVE_SNORENOTIFY)
    set(SOURCES ${SOURCES} snorenotificationbackend.cpp)
    set(FORMS ${FORMS} snorentificationconfigwidget.ui)
  endif(HAVE_SNORENOTIFY)
  if(HAVE_PHONON)
    set(SOURCES ${SOURCES} phononnotificationbackend.cpp)
    set(FORMS ${FORMS} phononnotificationconfigwidget.ui)
    include_directories(${PHONON_INCLUDES})
  endif(HAVE_PHONON)
endif(HAVE_KDE)

if (HAVE_DBUS)
    set(SOURCES ${SOURCES} statusnotifieritem.cpp statusnotifieritemdbus.cpp dockmanagernotificationbackend.cpp)
    qt_add_dbus_interface(DBUS ../../interfaces/org.kde.StatusNotifierWatcher.xml statusnotifierwatcher)
    qt_add_dbus_interface(DBUS ../../interfaces/org.freedesktop.Notifications.xml notificationsclient)
    qt_add_dbus_adaptor(DBUS ../../interfaces/org.kde.StatusNotifierItem.xml statusnotifieritemdbus.h StatusNotifierItemDBus)
endif(HAVE_DBUS)

if(HAVE_SSL)
  set(SOURCES ${SOURCES} sslinfodlg.cpp)
  set(FORMS ${FORMS} sslinfodlg.ui)
endif(HAVE_SSL)

if(INDICATEQT_FOUND)
  set(SOURCES ${SOURCES} indicatornotificationbackend.cpp)
  set(FORMS ${FORMS} indicatornotificationconfigwidget.ui)
  include_directories(${INDICATEQT_INCLUDE_DIRS})
endif(INDICATEQT_FOUND)

if(HAVE_NOTIFICATION_CENTER)
  set(SOURCES ${SOURCES} osxnotificationbackend.mm)
endif()

foreach(FORM ${FORMS})
  set(FORMPATH ${FORMPATH} ui/${FORM})
endforeach(FORM ${FORMS})

# handle settingspages
include(settingspages/settingspages.cmake)
foreach(SP ${SETTINGSPAGES})
  set(SPSRC ${SPSRC} settingspages/${SP}settingspage.cpp)
  set(SPFRM ${SPFRM} settingspages/${SP}settingspage.ui)
endforeach(SP ${SETTINGSPAGES})
foreach(SRC ${SP_SOURCES})
  set(SPSRC ${SPSRC} settingspages/${SRC})
endforeach(SRC ${SP_SOURCES})
foreach(FRM ${SP_FORMS})
  set(SPFRM ${SPFRM} settingspages/${FRM})
endforeach(FRM ${SP_FORMS})

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
                    ${CMAKE_SOURCE_DIR}/src/qtui
                    ${CMAKE_SOURCE_DIR}/src/qtui/settingspages
                    ${CMAKE_SOURCE_DIR}/src/uisupport

)

qt_wrap_ui(UI ${FORMPATH} ${SPFRM})

if (USE_QT5)
    list(APPEND qt_modules Widgets)
endif()

if (HAVE_DBUS)
    list(APPEND qt_modules DBus)
endif()

if (HAVE_WEBKIT)
    list(APPEND qt_modules Webkit XmlPatterns)
endif()

add_library(mod_qtui STATIC ${SOURCES} ${SPSRC} ${DBUS} ${UI})
qt_use_modules(mod_qtui Core Gui Network ${qt_modules})

add_dependencies(mod_qtui mod_common mod_client mod_uisupport)

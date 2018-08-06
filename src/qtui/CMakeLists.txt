quassel_add_module(QtUi)

target_sources(${TARGET} PRIVATE
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
    coresessionwidget.cpp
    debugbufferviewoverlay.cpp
    debugconsole.cpp
    debuglogdlg.cpp
    debugmessagemodelfilter.cpp
    inputwidget.cpp
    ircconnectionwizard.cpp
    legacysystemtray.cpp
    mainpage.cpp
    mainwin.cpp
    markerlineitem.cpp
    msgprocessorstatuswidget.cpp
    nicklistwidget.cpp
    passwordchangedlg.cpp
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
    systrayanimationnotificationbackend.cpp
    systraynotificationbackend.cpp
    taskbarnotificationbackend.cpp
    titlesetter.cpp
    topicwidget.cpp
    verticaldock.cpp
    webpreviewitem.cpp

    # UI files
    aboutdlg.ui
    bufferviewwidget.ui
    bufferwidget.ui
    channellistdlg.ui
    chatviewsearchbar.ui
    coreconfigwizardintropage.ui
    coreconfigwizardauthenticationselectionpage.ui
    coreconfigwizardadminuserpage.ui
    coreconfigwizardstorageselectionpage.ui
    coreconfigwizardsyncpage.ui
    coreconnectauthdlg.ui
    coreconnectionstatuswidget.ui
    coreinfodlg.ui
    coresessionwidget.ui
    debugbufferviewoverlay.ui
    debugconsole.ui
    debuglogdlg.ui
    inputwidget.ui
    msgprocessorstatuswidget.ui
    nicklistwidget.ui
    passwordchangedlg.ui
    receivefiledlg.ui
    settingsdlg.ui
    settingspagedlg.ui
    simplenetworkeditor.ui
    systrayanimationconfigwidget.ui
    topicwidget.ui
)

# Handle settingspages
target_include_directories(${TARGET} PRIVATE settingspages)
include(settingspages/settingspages.cmake)
foreach(page ${SETTINGSPAGES})
    target_sources(${TARGET} PRIVATE
        settingspages/${page}settingspage.cpp
        settingspages/${page}settingspage.ui
    )
endforeach()
foreach(src ${SP_SOURCES})
    target_sources(${TARGET} PRIVATE settingspages/${src})
endforeach()
foreach(form ${SP_FORMS})
    target_sources(${TARGET} PRIVATE settingspages/${form})
endforeach()

target_link_libraries(${TARGET}
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
        Quassel::UiSupport
        Quassel::Client
        Quassel::Common
)

if (WITH_KDE)
    target_sources(${TARGET} PRIVATE knotificationbackend.cpp)
    target_link_libraries(${TARGET} PUBLIC
        KF5::ConfigWidgets
        KF5::Notifications
        KF5::NotifyConfig
        KF5::WidgetsAddons
        KF5::XmlGui
    )
endif()

if (KF5Sonnet_FOUND)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_SONNET)
    target_sources(${TARGET} PRIVATE settingspages/sonnetsettingspage.cpp)
    target_link_libraries(${TARGET} PRIVATE KF5::SonnetUi)
endif()

if (Qt5Multimedia_FOUND)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_QTMULTIMEDIA)
    target_sources(${TARGET} PRIVATE
        qtmultimedianotificationbackend.cpp
        qtmultimedianotificationconfigwidget.ui
    )
    target_link_libraries(${TARGET} PRIVATE Qt5::Multimedia)
endif()

if (Qt5DBus_FOUND)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_DBUS)
    target_link_libraries(${TARGET} PRIVATE Qt5::DBus)
    if (dbusmenu-qt5_FOUND)
        target_compile_definitions(${TARGET} PRIVATE -DHAVE_DBUSMENU)
        target_link_libraries(${TARGET} PRIVATE dbusmenu-qt5)
    endif()

    qt5_add_dbus_interface(DBUS_SOURCES ../../interfaces/org.kde.StatusNotifierWatcher.xml statusnotifierwatcher)
    qt5_add_dbus_interface(DBUS_SOURCES ../../interfaces/org.freedesktop.Notifications.xml notificationsclient)
    qt5_add_dbus_adaptor  (DBUS_SOURCES ../../interfaces/org.kde.StatusNotifierItem.xml statusnotifieritemdbus.h StatusNotifierItemDBus)

    target_sources(${TARGET} PRIVATE
        dockmanagernotificationbackend.cpp
        statusnotifieritem.cpp
        statusnotifieritemdbus.cpp
        ${DBUS_SOURCES}
    )
endif()

if (HAVE_WEBKIT)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_WEBKIT)
    target_link_libraries(${TARGET} PRIVATE
        Qt5::WebKit
        Qt5::WebKitWidgets
    )
endif()

if (HAVE_WEBENGINE)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_WEBENGINE)
    target_link_libraries(${TARGET} PRIVATE
        Qt5::WebEngine
        Qt5::WebEngineWidgets)
endif()

if (HAVE_SSL)
    target_sources(${TARGET} PRIVATE
        sslinfodlg.cpp
        sslinfodlg.ui
    )
endif()

if (LibsnoreQt5_FOUND)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_LIBSNORE)
    target_sources(${TARGET} PRIVATE
        snorenotificationbackend.cpp
        snorentificationconfigwidget.ui
    )
    target_link_libraries(${TARGET} PRIVATE Snore::Libsnore Snore::LibsnoreSettings)
endif()

if (WITH_NOTIFICATION_CENTER)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_NOTIFICATION_CENTER)
    target_sources(${TARGET} PRIVATE osxnotificationbackend.mm)
    target_link_libraries(${TARGET} PRIVATE "/System/Library/Frameworks/Foundation.framework")
endif()

if (WITH_OXYGEN_ICONS)
    set_property(SOURCE qtui.cpp APPEND PROPERTY COMPILE_DEFINITIONS WITH_OXYGEN_ICONS)
endif()

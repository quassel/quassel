quassel_add_module(QtUi EXPORT)

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
    resourcetreedlg.cpp
    settingsdlg.cpp
    settingspagedlg.cpp
    simplenetworkeditor.cpp
    sslinfodlg.cpp
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
    debuglogdlg.ui
    inputwidget.ui
    msgprocessorstatuswidget.ui
    nicklistwidget.ui
    passwordchangedlg.ui
    receivefiledlg.ui
    resourcetreedlg.ui
    settingsdlg.ui
    settingspagedlg.ui
    simplenetworkeditor.ui
    sslinfodlg.ui
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
        ${QUASSEL_QT_CORE_TARGET}
        ${QUASSEL_QT_GUI_TARGET}
        ${QUASSEL_QT_NETWORK_TARGET}
        ${QUASSEL_QT_WIDGETS_TARGET}
        Quassel::UiSupport
        Quassel::Client
        Quassel::Common
)

if (WITH_KDE)
    target_sources(${TARGET} PRIVATE knotificationbackend.cpp)
    target_link_libraries(${TARGET} PUBLIC
        ${QUASSEL_KF_CONFIGWIDGETS_TARGET}
        ${QUASSEL_KF_NOTIFICATIONS_TARGET}
        ${QUASSEL_KF_NOTIFYCONFIG_TARGET}
        ${QUASSEL_KF_WIDGETSADDONS_TARGET}
        ${QUASSEL_KF_XMLGUI_TARGET}
    )
endif()

if (QUASSEL_HAVE_KF_SONNET)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_SONNET)
    target_sources(${TARGET} PRIVATE settingspages/sonnetsettingspage.cpp)
    target_link_libraries(${TARGET} PRIVATE ${QUASSEL_KF_SONNETUI_TARGET})
endif()

if (QUASSEL_HAVE_QT_MULTIMEDIA)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_QTMULTIMEDIA)
    target_sources(${TARGET} PRIVATE
        qtmultimedianotificationbackend.cpp
        qtmultimedianotificationconfigwidget.ui
    )
    target_link_libraries(${TARGET} PRIVATE ${QUASSEL_QT_MULTIMEDIA_TARGET})
endif()

if (QUASSEL_HAVE_QT_DBUS)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_DBUS)
    target_link_libraries(${TARGET} PRIVATE ${QUASSEL_QT_DBUS_TARGET})
    quassel_qt_add_dbus_interface(DBUS_SOURCES ../../interfaces/org.kde.StatusNotifierWatcher.xml statusnotifierwatcher)
    quassel_qt_add_dbus_interface(DBUS_SOURCES ../../interfaces/org.freedesktop.Notifications.xml notificationsclient)
    quassel_qt_add_dbus_adaptor  (DBUS_SOURCES ../../interfaces/org.kde.StatusNotifierItem.xml statusnotifieritemdbus.h StatusNotifierItemDBus)

    target_sources(${TARGET} PRIVATE
        dockmanagernotificationbackend.cpp
        statusnotifieritem.cpp
        statusnotifieritemdbus.cpp
        ${DBUS_SOURCES}
    )
endif()

if (QUASSEL_HAVE_WEBENGINE)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_WEBENGINE)
    target_link_libraries(${TARGET} PRIVATE
        ${QUASSEL_QT_WEBENGINE_TARGET}
        ${QUASSEL_QT_WEBENGINEWIDGETS_TARGET})
endif()

if (QUASSEL_HAVE_LIBSNORE)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_LIBSNORE)
    target_sources(${TARGET} PRIVATE
        snorenotificationbackend.cpp
        snorentificationconfigwidget.ui
    )
    target_link_libraries(${TARGET} PRIVATE ${QUASSEL_LIBSNORE_TARGET} ${QUASSEL_LIBSNORE_SETTINGS_TARGET})
endif()

if (WITH_NOTIFICATION_CENTER)
    target_compile_definitions(${TARGET} PRIVATE -DHAVE_NOTIFICATION_CENTER)
    target_sources(${TARGET} PRIVATE osxnotificationbackend.mm)
    target_link_libraries(${TARGET} PRIVATE "/System/Library/Frameworks/Foundation.framework")
endif()

if (WITH_OXYGEN_ICONS)
    set_property(SOURCE qtui.cpp APPEND PROPERTY COMPILE_DEFINITIONS WITH_OXYGEN_ICONS)
endif()

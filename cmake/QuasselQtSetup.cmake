# Centralized Qt/KDE package discovery and target naming for Quassel.

include_guard(GLOBAL)

set(QUASSEL_QT_MIN_VERSION "6.4.0")

set(QUASSEL_QT_CORE_TARGET Qt6::Core)
set(QUASSEL_QT_CORE5COMPAT_TARGET Qt6::Core5Compat)
set(QUASSEL_QT_NETWORK_TARGET Qt6::Network)
set(QUASSEL_QT_GUI_TARGET Qt6::Gui)
set(QUASSEL_QT_WIDGETS_TARGET Qt6::Widgets)
set(QUASSEL_QT_SQL_TARGET Qt6::Sql)
set(QUASSEL_QT_TEST_TARGET Qt6::Test)
set(QUASSEL_QT_DBUS_TARGET Qt6::DBus)
set(QUASSEL_QT_MULTIMEDIA_TARGET Qt6::Multimedia)
set(QUASSEL_QT_WEBENGINE_TARGET Qt6::WebEngineCore)
set(QUASSEL_QT_WEBENGINEWIDGETS_TARGET Qt6::WebEngineWidgets)
set(QUASSEL_QT_RCC_TARGET Qt6::rcc)
set(QUASSEL_QT_QMAKE_TARGET Qt6::qmake)

set(QUASSEL_KF_COREADDONS_TARGET KF6::CoreAddons)
set(QUASSEL_KF_CONFIGWIDGETS_TARGET KF6::ConfigWidgets)
set(QUASSEL_KF_NOTIFICATIONS_TARGET KF6::Notifications)
set(QUASSEL_KF_NOTIFYCONFIG_TARGET KF6::NotifyConfig)
set(QUASSEL_KF_SONNETUI_TARGET KF6::SonnetUi)
set(QUASSEL_KF_TEXTWIDGETS_TARGET KF6::TextWidgets)
set(QUASSEL_KF_WIDGETSADDONS_TARGET KF6::WidgetsAddons)
set(QUASSEL_KF_XMLGUI_TARGET KF6::XmlGui)

set(QUASSEL_QCA_TARGET qca-qt6)

set(QUASSEL_HAVE_QT_DBUS FALSE)
set(QUASSEL_HAVE_QT_MULTIMEDIA FALSE)
set(QUASSEL_HAVE_WEBENGINE FALSE)
set(QUASSEL_HAVE_KF_SONNET FALSE)
set(QUASSEL_HAVE_QCA FALSE)
set(WITH_KF FALSE)

# Required Qt components
set(quassel_qt_components Core Core5Compat Network CoreTools)
if (BUILD_GUI)
    list(APPEND quassel_qt_components Gui Widgets)
endif()
if (BUILD_CORE)
    list(APPEND quassel_qt_components Sql)
endif()

find_package(Qt6 ${QUASSEL_QT_MIN_VERSION} REQUIRED COMPONENTS ${quassel_qt_components})
set_package_properties(Qt6 PROPERTIES TYPE REQUIRED
    URL "https://www.qt.io/"
    DESCRIPTION "the Qt 6 libraries"
)
set(QUASSEL_QT_VERSION "${Qt6Core_VERSION}")
message(STATUS "Found Qt ${QUASSEL_QT_VERSION}")

if (APPLE)
    if (CMAKE_OSX_DEPLOYMENT_TARGET)
        message(STATUS "Configured macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
endif()

# Check for SSL support in Qt
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LIBRARIES ${QUASSEL_QT_CORE_TARGET} ${QUASSEL_QT_NETWORK_TARGET})
set(CMAKE_REQUIRED_FLAGS -std=c++17)
check_cxx_source_compiles("
    #include <QSslSocket>
    int main() {
        QSslSocket sock;
        return 0;
    }"
    HAVE_SSL)
cmake_pop_check_state()

if (NOT HAVE_SSL)
    message(FATAL_ERROR "Quassel requires SSL support, but Qt is built with QT_NO_SSL")
endif()

# Optional Qt components
find_package(Qt6 ${QUASSEL_QT_MIN_VERSION} QUIET COMPONENTS LinguistTools)
set_package_properties(Qt6LinguistTools PROPERTIES TYPE RECOMMENDED
    DESCRIPTION "contains tools for handling translation files"
    PURPOSE "Required for having translations"
)

if (BUILD_GUI)
    if (NOT WIN32)
        find_package(Qt6 ${QUASSEL_QT_MIN_VERSION} QUIET COMPONENTS DBus)
        set_package_properties(Qt6DBus PROPERTIES TYPE RECOMMENDED
            URL "https://www.qt.io/"
            DESCRIPTION "D-Bus support for Qt 6"
            PURPOSE     "Needed for supporting D-Bus-based notifications and tray icon, used by most modern desktop environments"
        )
        if (Qt6DBus_FOUND)
            set(QUASSEL_HAVE_QT_DBUS TRUE)
        endif()
    endif()

    find_package(Qt6 ${QUASSEL_QT_MIN_VERSION} QUIET COMPONENTS Multimedia)
    set_package_properties(Qt6Multimedia PROPERTIES TYPE RECOMMENDED
        URL "https://www.qt.io/"
        DESCRIPTION "Multimedia support for Qt 6"
        PURPOSE     "Required for audio notifications"
    )
    if (Qt6Multimedia_FOUND)
        set(QUASSEL_HAVE_QT_MULTIMEDIA TRUE)
    endif()

    if (WITH_WEBENGINE)
        find_package(Qt6 ${QUASSEL_QT_MIN_VERSION} REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)
        set_package_properties(Qt6WebEngineCore PROPERTIES TYPE REQUIRED
            URL "https://www.qt.io/"
            DESCRIPTION "the core WebEngine implementation for Qt"
            PURPOSE     "Needed for displaying previews for URLs in chat"
        )
        set_package_properties(Qt6WebEngineWidgets PROPERTIES TYPE REQUIRED
            URL "https://www.qt.io/"
            DESCRIPTION "widgets for Qt's WebEngine implementation"
            PURPOSE     "Needed for displaying previews for URLs in chat"
        )
        set(QUASSEL_HAVE_WEBENGINE TRUE)
    endif()
    add_feature_info("WITH_WEBENGINE, QtWebEngine and QtWebEngineWidgets modules" QUASSEL_HAVE_WEBENGINE "Support showing previews for URLs in chat")

    # KDE Frameworks
    ################

    # extra-cmake-modules
    if (WITH_KDE)
        set(ecm_find_type "REQUIRED")
        find_package(ECM NO_MODULE REQUIRED)
    else()
        # Even with KDE integration disabled, we optionally use tier1 frameworks if we find them
        set(ecm_find_type "RECOMMENDED")
        find_package(ECM NO_MODULE QUIET)
    endif()

    set_package_properties(ECM PROPERTIES TYPE ${ecm_find_type}
        URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules"
        DESCRIPTION "extra modules for CMake, maintained by the KDE project"
        PURPOSE     "Required to find KDE Frameworks components"
    )

    if (ECM_FOUND)
        list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
        if (WITH_KDE)
            find_package(KF6 REQUIRED COMPONENTS ConfigWidgets CoreAddons Notifications NotifyConfig Sonnet TextWidgets WidgetsAddons XmlGui)
            set_package_properties(KF6 PROPERTIES TYPE REQUIRED
                URL "http://www.kde.org"
                DESCRIPTION "KDE Frameworks 6"
                PURPOSE     "Required for integration into the Plasma desktop"
            )
            message(STATUS "Found KDE Frameworks ${KF6_VERSION}")
            set(WITH_KF TRUE)
        endif()

        # Optional KF6 tier1 components
        find_package(KF6Sonnet QUIET)
        set_package_properties(KF6Sonnet PROPERTIES TYPE RECOMMENDED
            URL "http://api.kde.org/frameworks-api/frameworks6-apidocs/sonnet/html"
            DESCRIPTION "framework for providing spell-checking capabilities"
            PURPOSE "Enables spell-checking support in input widgets"
        )
        if (KF6Sonnet_FOUND)
            set(QUASSEL_HAVE_KF_SONNET TRUE)
        endif()
    endif()
endif()

if (BUILD_CORE)
    find_package(Qca-qt6 2.0 QUIET)
    set_package_properties(Qca-qt6 PROPERTIES TYPE RECOMMENDED
        URL "https://projects.kde.org/projects/kdesupport/qca"
        DESCRIPTION "Qt Cryptographic Architecture"
        PURPOSE "Required for encryption support"
    )
    if (Qca-qt6_FOUND)
        set(QUASSEL_HAVE_QCA TRUE)
    endif()

    if (WITH_LDAP)
        find_package(Ldap QUIET)
        set_package_properties(Ldap PROPERTIES TYPE OPTIONAL
            URL "http://www.openldap.org/"
            DESCRIPTION "LDAP (Lightweight Directory Access Protocol) libraries"
            PURPOSE "Enables core user authentication via LDAP"
        )
    endif()
endif()

function(quassel_qt_add_dbus_interface outvar xml basename)
    qt6_add_dbus_interface(${outvar} ${xml} ${basename} ${ARGN})
    set(${outvar} ${${outvar}} PARENT_SCOPE)
endfunction()

function(quassel_qt_add_dbus_adaptor outvar xml header parent_class)
    qt6_add_dbus_adaptor(${outvar} ${xml} ${header} ${parent_class} ${ARGN})
    set(${outvar} ${${outvar}} PARENT_SCOPE)
endfunction()

# Install icons

if (WANT_MONO OR WANT_QTCLIENT)
    if (NOT WITH_BUNDLED_ICONS AND NOT WITH_OXYGEN_ICONS)
        message(WARNING "Not including bundled icon themes. Ensure that the Breeze or Oxygen icon theme is installed!")
    endif()

    set(BUNDLED_ICON_THEME_DIR ${CMAKE_SOURCE_DIR}/3rdparty/icons)

    # Always embed a hicolor fallback containing the most important (e.g. tray) icons
    list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/hicolor_icons.qrc)

    if (EMBED_DATA)
        # Always embed quassel-specific icons
        message(STATUS "Embedding Quassel-specific icons")
        list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/breeze_icons.qrc)
        list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/breeze_dark_icons.qrc)
        if (WITH_OXYGEN_ICONS)
            list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/oxygen_icons.qrc)
        endif()

        if (WITH_BUNDLED_ICONS)
            message(STATUS "Embedding bundled Breeze icon theme")
            list(APPEND ICON_RCS ${BUNDLED_ICON_THEME_DIR}/breeze_icon_theme.qrc)
            list(APPEND ICON_RCS ${BUNDLED_ICON_THEME_DIR}/breeze_dark_icon_theme.qrc)
            if (WITH_OXYGEN_ICONS)
                message(STATUS "Embedding bundled Oxygen icon theme")
                list(APPEND ICON_RCS ${BUNDLED_ICON_THEME_DIR}/oxygen_icon_theme.qrc)
            endif()
        endif()

    else()
        # Always install quassel-specific icons
        install(DIRECTORY breeze breeze-dark DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        if (WITH_OXYGEN_ICONS)
            install(DIRECTORY oxygen DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()

        # Install bundled icon themes into our data dir (instead of CMAKE_INSTALL_ICONDIR) to avoid conflicts
        # with themes provided by the system
        if (WITH_BUNDLED_ICONS)
            message(STATUS "Installing bundled Breeze icon theme")
            install(DIRECTORY   ${BUNDLED_ICON_THEME_DIR}/breeze
                                ${BUNDLED_ICON_THEME_DIR}/breeze-dark
                    DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons
            )
            if (WITH_OXYGEN_ICONS)
                message(STATUS "Installing bundled Oxygen icon theme")
                install(DIRECTORY ${BUNDLED_ICON_THEME_DIR}/oxygen DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
            endif()
        endif()
    endif()

    set(CLIENT_RCS ${CLIENT_RCS} ${ICON_RCS} PARENT_SCOPE)

    # Application icon
    if (HAVE_KDE OR (UNIX AND NOT APPLE))
        # hicolor contains the application icon in all relevant sizes
        install(DIRECTORY hicolor DESTINATION ${CMAKE_INSTALL_ICONDIR})

        if (USE_QT4 OR (USE_QT5 AND Qt5Core_VERSION VERSION_LESS 5.5.0))
            # Qt 4 doesn't seem to correctly load icons from injected themes
            # Install Quassel-specific ones from Oxygen into hicolor as fallback
            install(DIRECTORY oxygen/ DESTINATION ${CMAKE_INSTALL_ICONDIR}/hicolor)
        endif()

        # For a system install, also copy to pixmaps
        if (CMAKE_INSTALL_PREFIX STREQUAL "/usr")
            install(FILES hicolor/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
        endif()
    endif()
endif()

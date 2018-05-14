# Install icons

if (WANT_MONO OR WANT_QTCLIENT)
    set(BUNDLED_ICON_THEME_DIR ${CMAKE_SOURCE_DIR}/3rdparty/icons)

    if (EMBED_DATA)
        # Always embed quassel-specific icons
        message(STATUS "Embedding Quassel-specific icons")
        list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/breeze_icons.qrc)
        list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/breeze_dark_icons.qrc)
        list(APPEND ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/oxygen_icons.qrc)

        if (WITH_BREEZE)
            message(STATUS "Embedding bundled Breeze icon theme")
            list(APPEND ICON_RCS ${BUNDLED_ICON_THEME_DIR}/breeze_icon_theme.qrc)
        endif()
        if (WITH_BREEZE_DARK)
            message(STATUS "Embedding bundled Breeze Dark icon theme")
            list(APPEND ICON_RCS ${BUNDLED_ICON_THEME_DIR}/breeze_dark_icon_theme.qrc)
        endif()
        if (WITH_OXYGEN)
            message(STATUS "Embedding bundled Oxygen icon theme")
            list(APPEND ICON_RCS ${BUNDLED_ICON_THEME_DIR}/oxygen_icon_theme.qrc)
        endif()

    else()
        # Always install quassel-specific icons
        install(DIRECTORY breeze breeze-dark oxygen DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)

        # Install bundled icon themes into our data dir (instead of CMAKE_INSTALL_ICONDIR) to avoid conflicts
        # with themes provided by the system
        if (WITH_BREEZE)
            message(STATUS "Installing bundled Breeze icon theme")
            install(DIRECTORY ${BUNDLED_ICON_THEME_DIR}/breeze DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()
        if (WITH_BREEZE_DARK)
            message(STATUS "Installing bundled Breeze Dark icon theme")
            install(DIRECTORY ${BUNDLED_ICON_THEME_DIR}/breeze-dark DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()
        if (WITH_OXYGEN)
            message(STATUS "Installing bundled Oxygen icon theme")
            install(DIRECTORY ${BUNDLED_ICON_THEME_DIR}/oxygen DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()
    endif()

    set(CLIENT_RCS ${CLIENT_RCS} ${ICON_RCS} PARENT_SCOPE)

    # Application icon
    if (HAVE_KDE OR (UNIX AND NOT APPLE))
        # hicolor contains the application icon in all relevant sizes
        install(DIRECTORY hicolor DESTINATION ${CMAKE_INSTALL_ICONDIR})

        # For a system install, also copy to pixmaps
        if (CMAKE_INSTALL_PREFIX STREQUAL "/usr")
            install(FILES hicolor/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
        endif()
    endif()
endif()

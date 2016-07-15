# Install icons
#
# We put the bundled oxygen icon theme in DATADIR rather than ICONDIR,
# in order to avoid conflicts. As we add this directory to XDG_DATA_DIRS at runtime,
# the bundled theme will be found by Qt only if there is no oxygen theme installed
# in system directories.
#
# The hicolor folder contains icons vital for the Quassel UI (such as in the channel and nick
# lists). It is installed into the system icon directory (in order to allow overriding) and
# additionally always embedded as a fallback resource.

if (WANT_MONO OR WANT_QTCLIENT)
    set(ICON_RCS ${CMAKE_CURRENT_SOURCE_DIR}/hicolor.qrc) # always embed those

    if (EMBED_DATA)
        if (WITH_OXYGEN)
            message(STATUS "Embedding bundled Oxygen icons")
            set(ICON_RCS ${ICON_RCS} ${CMAKE_CURRENT_SOURCE_DIR}/oxygen.qrc)
        endif()
        if (WITH_BREEZE)
            message(STATUS "Embedding bundled Breeze icons")
            set(ICON_RCS ${ICON_RCS} ${CMAKE_CURRENT_SOURCE_DIR}/breeze.qrc)
        endif()
        if (WITH_BREEZE_DARK)
            message(STATUS "Embedding bundled Breeze Dark icons")
            set(ICON_RCS ${ICON_RCS} ${CMAKE_CURRENT_SOURCE_DIR}/breezedark.qrc)
        endif()
    else()
        install(DIRECTORY hicolor DESTINATION ${CMAKE_INSTALL_ICONDIR})
        if (WITH_OXYGEN)
            message(STATUS "Installing bundled Oxygen icons")
            install(DIRECTORY oxygen DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()
        if (WITH_BREEZE)
            message(STATUS "Installing bundled Breeze icons")
            install(DIRECTORY breeze DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()
        if (WITH_BREEZE_DARK)
            message(STATUS "Installing bundled Breeze Dark icons")
            install(DIRECTORY breezedark DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/icons)
        endif()
    endif()

    if (NOT WITH_OXYGEN)
        message(STATUS "Not installing bundled Oxygen icons")
    endif()

    if (NOT WITH_BREEZE)
        message(STATUS "Not installing bundled Breeze icons")
    endif()

    if (NOT WITH_BREEZE_DARK)
        message(STATUS "Not installing bundled Breeze Dark icons")
    endif()

    set(CLIENT_RCS ${CLIENT_RCS} ${ICON_RCS} PARENT_SCOPE)
endif()

# Application icon

if (HAVE_KDE OR (UNIX AND NOT APPLE))
    install(FILES hicolor/48x48/apps/quassel.png DESTINATION ${CMAKE_INSTALL_ICONDIR}/hicolor/48x48/apps)
    if (CMAKE_INSTALL_PREFIX STREQUAL "/usr")
        install(FILES hicolor/48x48/apps/quassel.png DESTINATION /usr/share/pixmaps)
    endif()
endif()

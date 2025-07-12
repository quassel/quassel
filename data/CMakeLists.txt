if (BUILD_GUI)
    if (WITH_${KF_FULL})
        set(DESKTOP_FILE "quasselclient")

        if (WANT_MONO)
            set(DESKTOP_FILE "quassel")
        endif()

        configure_file(quassel.notifyrc.in quassel.notifyrc @ONLY)

        install(FILES ${CMAKE_BINARY_DIR}/data/quassel.notifyrc DESTINATION ${CMAKE_INSTALL_KNOTIFY5RCDIR})
    endif()

    if (UNIX AND NOT APPLE)
        if (WANT_QTCLIENT)
            install(FILES quasselclient.desktop DESTINATION ${CMAKE_INSTALL_APPDIR})
        endif()

        if (WANT_MONO)
            install(FILES quassel.desktop DESTINATION ${CMAKE_INSTALL_APPDIR})
        endif()
    endif()

    if (EMBED_DATA)
        quassel_add_resource(Data PREFIX data PATTERNS
            networks.ini
            stylesheets/default.qss
        )
    else()
        install(FILES networks.ini DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel)
        install(DIRECTORY stylesheets DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel)
    endif()

    if (NOT WIN32)
        install(DIRECTORY scripts DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel USE_SOURCE_PERMISSIONS)
    endif()
endif()

if (BUILD_GUI)
    if (WITH_KF6)
        set(DESKTOP_FILE "quasselclient")

        if (WANT_MONO)
            set(DESKTOP_FILE "quassel")
        endif()

        configure_file(quassel.notifyrc.in quassel.notifyrc @ONLY)

        install(FILES ${CMAKE_BINARY_DIR}/data/quassel.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
    endif()

    if (UNIX AND NOT APPLE)
        if (WANT_QTCLIENT)
            if(WITH_KDE)
                install(FILES quasselclient.desktop DESTINATION ${KDE_INSTALL_APPDIR})
            else()
                install(FILES quasselclient.desktop DESTINATION ${CMAKE_INSTALL_APPDIR})
            endif()
        endif()

        if (WANT_MONO)
            if(WITH_KDE)
                install(FILES quassel.desktop DESTINATION ${KDE_INSTALL_APPDIR})
            else()
                install(FILES quassel.desktop DESTINATION ${CMAKE_INSTALL_APPDIR})
            endif()
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

if (BUILD_GUI)
    if (WITH_KF5)
        install(FILES quassel.notifyrc DESTINATION ${CMAKE_INSTALL_KNOTIFY5RCDIR})
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

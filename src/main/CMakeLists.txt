# Convenience function to avoid boilerplate
function(setup_executable _target _define)
    set_target_properties(${_target} PROPERTIES
        COMPILE_FLAGS ${_define}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    target_link_libraries(${_target} PRIVATE ${ARGN})
    install(TARGETS ${_target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

# We need to initialize the appropriate resources, so let's give our main.cpp some hints
if (EMBED_DATA)
    set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS EMBED_DATA)
endif()

if (HAVE_UMASK)
    set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS HAVE_UMASK)
endif()

if (WITH_BUNDLED_ICONS)
    set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS WITH_BUNDLED_ICONS)
endif()

# Windows icon resource
if(WIN32)
    if(MINGW)
        find_program(WINDRES_EXECUTABLE NAMES windres)
        if(WINDRES_EXECUTABLE)
            exec_program(windres
                ARGS "-i ${CMAKE_SOURCE_DIR}/pics/win32.rc"
                     "-o ${CMAKE_CURRENT_BINARY_DIR}/win32.o"
                     "--include-dir=${CMAKE_SOURCE_DIR}/pics"
                )
            set(WIN_RC ${CMAKE_CURRENT_BINARY_DIR}/win32.o)
        endif()
    else()
        set(WIN_RC ${CMAKE_SOURCE_DIR}/pics/win32.rc)
    endif()
endif()


# Build the executables
if (WANT_CORE)
    add_executable(quasselcore main.cpp ${WIN_RC})
    setup_executable(quasselcore -DBUILD_CORE Qt5::Core Quassel::Core)
endif()

if (WANT_CLIENT OR WANT_QTCLIENT)
    add_executable(quasselclient WIN32 main.cpp ${WIN_RC})
    setup_executable(quasselclient -DBUILD_QTUI Qt5::Core Qt5::Gui Quassel::QtUi)
    if (WITH_KDE)
        target_link_libraries(quasselclient PRIVATE KF5::CoreAddons)
    endif()
endif()

if (WANT_MONO)
    add_executable(quassel WIN32 main.cpp monoapplication.cpp ${WIN_RC})
    setup_executable(quassel -DBUILD_MONO Qt5::Core Qt5::Gui Quassel::Core Quassel::QtUi)
    if (WITH_KDE)
        target_link_libraries(quassel PRIVATE KF5::CoreAddons)
    endif()
endif()

# Build bundles for MacOSX
if (APPLE)
    add_custom_command(TARGET quasselclient POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makebundle.py
                               ${CMAKE_SOURCE_DIR} "Quassel Client" ${CMAKE_BINARY_DIR}/quasselclient)

    add_custom_command(TARGET quassel POST_BUILD
                       COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makebundle.py
                               ${CMAKE_SOURCE_DIR} "Quassel" ${CMAKE_BINARY_DIR}/quassel)
    if (DEPLOY)
        add_custom_command(TARGET quasselclient POST_BUILD WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                           COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Client ${CMAKE_BINARY_DIR} qsvgicon)
        add_custom_command(TARGET quasselcore POST_BUILD WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                           COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Core ${CMAKE_BINARY_DIR})
        add_custom_command(TARGET quassel POST_BUILD WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                           COMMAND ${CMAKE_SOURCE_DIR}/scripts/build/macosx_makePackage.sh Mono ${CMAKE_BINARY_DIR} qsvgicon)
    endif()
endif()

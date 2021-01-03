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
    quassel_add_executable(quasselcore COMPONENT Core SOURCES main.cpp ${WIN_RC} LIBRARIES Qt5::Core Quassel::Core)
endif()

if (WANT_QTCLIENT)
    set(libs Qt5::Core Qt5::Gui Quassel::QtUi)
    if (WITH_KDE)
        list(APPEND libs KF5::CoreAddons)
    endif()
    quassel_add_executable(quasselclient COMPONENT Client SOURCES main.cpp ${WIN_RC} LIBRARIES ${libs})
endif()

if (WANT_MONO)
    set(libs Qt5::Core Qt5::Gui Quassel::Core Quassel::QtUi)
    if (WITH_KDE)
        list(APPEND libs KF5::CoreAddons)
    endif()
    quassel_add_executable(quassel COMPONENT Mono SOURCES main.cpp monoapplication.cpp ${WIN_RC} LIBRARIES ${libs})
endif()

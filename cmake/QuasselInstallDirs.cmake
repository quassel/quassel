# This file sets up install locations for Quassel
#
# (C) 2015 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


# Setup install locations
#####################################################################

# We support the CMake standard variables (CMAKE_INSTALL_<TYPE>_DIR),
# which can be overridden on the command line. If KDE integration is enabled,
# we make use of its settings.

if (NOT WITH_KDE)
    if (WIN32)
        # On Windows, we have to guess good paths
        # We must check if the variables are already defined on the command line
        if (NOT DEFINED CMAKE_INSTALL_BINDIR)
            set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Install path for binaries")
        endif()
        if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
            set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Install path for libraries")
        endif()
        if (NOT DEFINED CMAKE_INSTALL_DATADIR)
            set(CMAKE_INSTALL_DATADIR "$ENV{APPDATA}/quassel-irc.org/share/apps" CACHE PATH "Install path for data files")
        endif()
        if (NOT DEFINED CMAKE_INSTALL_ICONDIR)
            set(CMAKE_INSTALL_ICONDIR "$ENV{APPDATA}/quassel-irc.org/share/icons" CACHE PATH "Install path for icons")
        endif()
        if (NOT DEFINED CMAKE_INSTALL_APPDIR)
            set(CMAKE_INSTALL_APPDIR "$ENV{APPDATA}/quassel-irc.org/share/applications" CACHE PATH "Install path for .desktop files")
        endif()
    else()
        # This sets the standard variables; however it doesn't know about icons and apps
        include(GNUInstallDirs)

        # Special treatment for paths relative to DATAROOTDIR, as found in GNUInstallDirs
        if (NOT CMAKE_INSTALL_ICONDIR)
            set(CMAKE_INSTALL_ICONDIR "" CACHE PATH "Install path for icons")
            set(CMAKE_INSTALL_ICONDIR "${CMAKE_INSTALL_DATAROOTDIR}/icons")
        endif()
        if (NOT CMAKE_INSTALL_APPDIR)
            set(CMAKE_INSTALL_APPDIR "" CACHE PATH "Install path for .desktop files")
            set(CMAKE_INSTALL_APPDIR "${CMAKE_INSTALL_DATAROOTDIR}/applications")
        endif()
    endif()
endif()

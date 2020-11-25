#!/usr/bin/python
# -*- coding: iso-8859-1 -*-

################################################################################
#                                                                              #
# 2008 June 27th by Marcus 'EgS' Eggenberger <egs@quassel-irc.org>             #
#                                                                              #
# The author disclaims copyright to this source code.                          #
# This Python Script is in the PUBLIC DOMAIN.                                  #
#                                                                              #
################################################################################

# ==============================
#  Imports
# ==============================
import os
from subprocess import Popen, PIPE

# ==============================
#  Global Functions
# ==============================
def qtProperty(qtProperty):
    """
    Query persistent property of Qt via qmake
    """
    VALID_PROPERTIES = ['QT_INSTALL_PREFIX',
                        'QT_INSTALL_DATA',
                        'QT_INSTALL_DOCS',
                        'QT_INSTALL_HEADERS',
                        'QT_INSTALL_LIBS',
                        'QT_INSTALL_BINS',
                        'QT_INSTALL_PLUGINS',
                        'QT_INSTALL_IMPORTS',
                        'QT_INSTALL_TRANSLATIONS',
                        'QT_INSTALL_CONFIGURATION',
                        'QT_INSTALL_EXAMPLES',
                        'QT_INSTALL_DEMOS',
                        'QMAKE_MKSPECS',
                        'QMAKE_VERSION',
                        'QT_VERSION'
                        ]
    if qtProperty not in VALID_PROPERTIES:
        return None

    qmakeProcess = Popen('qmake -query %s' % qtProperty, shell=True, stdout=PIPE, stderr=PIPE)
    result = qmakeProcess.stdout.read().strip()
    qmakeProcess.stdout.close()
    qmakeProcess.wait()
    return result

def qtMakespec(qtMakespec):
    """
    Query a Makespec value of Qt via qmake
    """

    VALID_PROPERTIES = ['QMAKE_MACOSX_DEPLOYMENT_TARGET',
                        ]
    if qtMakespec not in VALID_PROPERTIES:
        return None

    # QMAKE_MACOSX_DEPLOYMENT_TARGET sadly cannot be queried in the traditional way
    #
    # Inspired by https://code.qt.io/cgit/pyside/pyside-setup.git/tree/qtinfo.py?h=5.6
    # Simplified, no caching, etc, as we're just looking for the macOS version.
    # If a cleaner solution is desired, look into license compatibility in
    # order to simply copy the above code.

    current_dir = os.getcwd()
    qmakeFakeProjectFile = os.path.join(current_dir, "qmake_empty_project.txt")
    qmakeStashFile = os.path.join(current_dir, ".qmake.stash")
    # Make an empty file
    open(qmakeFakeProjectFile, 'a').close()

    qmakeProcess = Popen('qmake -E %s' % qmakeFakeProjectFile, shell=True, stdout=PIPE, stderr=PIPE)
    result = qmakeProcess.stdout.read().strip()
    qmakeProcess.stdout.close()
    qmakeProcess.wait()

    # Clean up temporary files
    try:
        os.remove(qmakeFakeProjectFile)
    except OSError:
        pass
    try:
        os.remove(qmakeStashFile)
    except OSError:
        pass

    # Result should be like this:
    # PROPERTY = VALUE\n
    result_list = result.splitlines()
    # Clear result so if nothing matches, nothing is returned
    result = None
    # Search keys
    for line in result_list:
        if not '=' in line:
            # Ignore lines without '='
            continue

        # Find property = value
        parts = line.split('=', 1)
        prop = parts[0].strip()
        value = parts[1].strip()
        if (prop == qtMakespec):
            result = value
            break

    return result

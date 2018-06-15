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
import os.path
import sys
import commands

# ==============================
#  Constants
# ==============================
if len(sys.argv) < 2:
    sys.exit(1)

SOURCE_DIR = sys.argv[1]

if len(sys.argv) < 4:
    BUNDLE_NAME = "Quassel Client"
    EXE_NAME = "quasselclient"
else:
    EXE_NAME = sys.argv[3]
    BUNDLE_NAME = sys.argv[2]

# make the dir of the exe the target dir
if(os.path.dirname(EXE_NAME)):
    CONTENTS_DIR = os.path.dirname(EXE_NAME) + "/"
CONTENTS_DIR += BUNDLE_NAME + ".app/Contents/"

BUNDLE_VERSION = commands.getoutput("git --git-dir=" + SOURCE_DIR + "/.git/ describe")
ICONSET_FOLDER = "pics/iconset/"


def createBundle():
    try:
        os.makedirs(CONTENTS_DIR + "MacOS")
        os.makedirs(CONTENTS_DIR + "Resources")
    except:
        pass


def copyFiles(exeFile, iconset):
    os.system("cp %s %sMacOs/%s" % (exeFile, CONTENTS_DIR.replace(' ', '\ '), BUNDLE_NAME.replace(' ', '\ ')))
    os.system("cp -r %s/%s %s/Resources/quassel.iconset/" % (SOURCE_DIR, iconset, CONTENTS_DIR.replace(' ', '\ ')))


def createPlist(bundleName, bundleVersion):
    templateFile = file(SOURCE_DIR + "/scripts/build/Info.plist", 'r')
    template = templateFile.read()
    templateFile.close()

    plistFile = file(CONTENTS_DIR + "Info.plist", 'w')
    plistFile.write(template % {"BUNDLE_NAME": bundleName,
                                "ICON_FILE": "quassel.icns",
                                "BUNDLE_VERSION": bundleVersion})
    plistFile.close()

def convertIconset():
    os.system("iconutil -c icns %s/Resources/quassel.iconset" % CONTENTS_DIR.replace(' ', '\ '))
    os.system("rm -R %s/Resources/quassel.iconset" % CONTENTS_DIR.replace(' ', '\ '))

if __name__ == "__main__":
    createBundle()
    createPlist(BUNDLE_NAME, BUNDLE_VERSION)
    copyFiles(EXE_NAME, ICONSET_FOLDER)
    convertIconset()

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
    BUNDLE_NAME= "Quassel Client"
    EXE_NAME = "quasselclient"
else:
    EXE_NAME = sys.argv[3]
    BUNDLE_NAME = sys.argv[2]

# make the dir of the exe the target dir
if(os.path.dirname(EXE_NAME)):
    CONTENTS_DIR = os.path.dirname(EXE_NAME) + "/"
CONTENTS_DIR += BUNDLE_NAME + ".app/Contents/"

BUNDLE_VERSION = commands.getoutput("git --git-dir="+SOURCE_DIR+"/.git/ describe")
ICON_FILE = "pics/quassel.icns"

def createBundle():
    try:
        os.makedirs(CONTENTS_DIR + "MacOS")
        os.makedirs(CONTENTS_DIR + "Resources")
    except:
        pass

def copyFiles(exeFile, iconFile):
    os.system("cp %s %sMacOs/%s" % (exeFile, CONTENTS_DIR.replace(' ', '\ '), BUNDLE_NAME.replace(' ', '\ ')))
    os.system("cp %s/%s %s/Resources" % (SOURCE_DIR, iconFile, CONTENTS_DIR.replace(' ', '\ ')))

def createPlist(bundleName, iconFile, bundleVersion):
    templateFile = file(SOURCE_DIR + "/scripts/build/Info.plist", 'r')
    template = templateFile.read()
    templateFile.close()

    plistFile = file(CONTENTS_DIR + "Info.plist", 'w')
    plistFile.write(template % {"BUNDLE_NAME" : bundleName,
                                "ICON_FILE" : iconFile[iconFile.rfind("/")+1:],
                                "BUNDLE_VERSION" : bundleVersion})
    plistFile.close()

if __name__ == "__main__":
    createBundle()
    createPlist(BUNDLE_NAME, ICON_FILE, BUNDLE_VERSION)
    copyFiles(EXE_NAME, ICON_FILE)
    pass

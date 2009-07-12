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
import sys
import os

from subprocess import Popen, PIPE

class InstallQt(object):
    def __init__(self, appdir, bundle = True):
        self.appDir = appdir
        self.bundle = bundle
        self.executableDir = self.appDir
        if bundle:
            self.executableDir += "/MacOS"

        if bundle:
            self.frameworkDir = self.appDir + "/Frameworks"
        else:
            self.frameworkDir = self.executableDir + "/Frameworks"

        self.needFrameworks = []

        executables = [self.executableDir + "/" + executable for executable in os.listdir(self.executableDir)]

        for executable in executables:
            for framework,lib in self.determineDependancies(executable):
                if framework not in self.needFrameworks:
                    self.needFrameworks.append(framework)
                    self.installFramework(framework)
            self.changeDylPath(executable)

    def installFramework(self, framework):
        try:
            os.mkdir(self.frameworkDir)
        except:
            pass

        # Copy Framework
        os.system('cp -R "%s" "%s"' % (framework, self.frameworkDir))

        frameworkname = framework.split('/')[-1]
        localframework = self.frameworkDir + "/" + frameworkname

        # De-Myllify
        os.system('find "%s" -name *debug* -exec rm -f {} \;' % localframework)
        os.system('find "%s" -name Headers -exec rm -rf {} \; 2>/dev/null' % localframework)

        # Install new Lib ID and Change Path to Frameworks for the Dynamic linker
        for lib in os.listdir(localframework + "/Versions/Current"):
            lib = "%s/Versions/Current/%s" % (localframework, lib)
            otoolProcess = Popen('otool -D "%s"' % lib, shell=True, stdout=PIPE, stderr=PIPE)
            try:
                libname = [line for line in otoolProcess.stdout][1].strip()
            except:
                libname = ''
            otoolProcess.stdout.close()
            if otoolProcess.wait() == 1: # we found some Resource dir or similar -> skip
                continue
            frameworkpath, libpath = libname.split(frameworkname)
            if self.bundle:
                newlibname = "@executable_path/../%s%s" % (frameworkname, libpath)
            else:
                newlibname = "@executable_path/%s%s" % (frameworkname, libpath)
            #print 'install_name_tool -id "%s" "%s"' % (newlibname, lib)
            os.system('install_name_tool -id "%s" "%s"' % (newlibname, lib))
            self.changeDylPath(lib)
            
    def determineDependancies(self, app):
        otoolPipe = Popen('otool -L "%s"' % app, shell=True, stdout=PIPE).stdout
        otoolOutput = [line for line in otoolPipe]
        otoolPipe.close()
        libs = [line.split()[0] for line in otoolOutput[1:] if "Qt" in line and not "@executable_path" in line]
        frameworks = [lib[:lib.find(".framework")+len(".framework")] for lib in libs]
        return zip(frameworks, libs)

    def changeDylPath(self, obj):
        for framework, lib in self.determineDependancies(obj):
            frameworkname = framework.split('/')[-1]
            frameworkpath, libpath = lib.split(frameworkname)
            if self.bundle:
                newlibname = "@executable_path/../Frameworks/%s%s" % (frameworkname, libpath)
            else:
                newlibname = "@executable_path/Frameworks/%s%s" % (frameworkname, libpath)

            #print 'install_name_tool -change "%s" "%s" "%s"' % (lib, newlibname, obj)
            os.system('install_name_tool -change "%s" "%s" "%s"' % (lib, newlibname, obj))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Wrong Argument Count (Syntax: %s [--nobundle] $TARGET_APP)" % sys.argv[0]
        sys.exit(1)
    else:
        bundle = True
        offset = 1

        if sys.argv[1].startswith("--"):
            offset = 2
            if sys.argv[1] == "--nobundle":
                bundle = False

        targetDir = sys.argv[offset]
        if bundle:
            targetDir += "/Contents"

        InstallQt(targetDir, bundle)
    
    

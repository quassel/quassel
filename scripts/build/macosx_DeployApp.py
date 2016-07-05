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
import os.path

from subprocess import Popen, PIPE

# ==============================
#  Constants
# ==============================
QT_CONFIG = """[Paths]
 Plugins = plugins
"""

QT_CONFIG_NOBUNDLE = """[Paths]
 Prefix = ../
 Plugins = plugins
"""


class InstallQt(object):
    def __init__(self, appdir, bundle=True, requestedPlugins=[]):
        self.appDir = appdir
        self.bundle = bundle
        self.frameworkDir = self.appDir + "/Frameworks"
        self.pluginDir = self.appDir + "/plugins"
        self.executableDir = self.appDir
        if bundle:
            self.executableDir += "/MacOS"

        self.installedFrameworks = set()

        self.findFrameworkPath()

        executables = [self.executableDir + "/" + executable for executable in os.listdir(self.executableDir)]
        for executable in executables:
            self.resolveDependancies(executable)

        self.findPluginsPath()
        self.installPlugins(requestedPlugins)
        self.installQtConf()

    def qtProperty(self, qtProperty):
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

    def findFrameworkPath(self):
        self.sourceFrameworkPath = self.qtProperty('QT_INSTALL_LIBS')

    def findPluginsPath(self):
        self.sourcePluginsPath = self.qtProperty('QT_INSTALL_PLUGINS')

    def findPlugin(self, pluginname):
        qmakeProcess = Popen('find %s -name %s' % (self.sourcePluginsPath, pluginname), shell=True, stdout=PIPE, stderr=PIPE)
        result = qmakeProcess.stdout.read().strip()
        qmakeProcess.stdout.close()
        qmakeProcess.wait()
        if not result:
            raise OSError
        return result

    def installPlugins(self, requestedPlugins):
        try:
            os.mkdir(self.pluginDir)
        except:
            pass

        for plugin in requestedPlugins:
            if not plugin.isalnum():
                print "Skipping library '%s'..." % plugin
                continue

            pluginName = "lib%s.dylib" % plugin
            pluginSource = ''
            try:
                pluginSource = self.findPlugin(pluginName)
            except OSError:
                print "WARNING: Requested library does not exist: '%s'" % plugin
                continue

            pluginSubDir = os.path.dirname(pluginSource)
            pluginSubDir = pluginSubDir.replace(self.sourcePluginsPath, '').strip('/')
            try:
                os.mkdir("%s/%s" % (self.pluginDir, pluginSubDir))
            except OSError:
                pass

            os.system('cp "%s" "%s/%s"' % (pluginSource, self.pluginDir, pluginSubDir))

            self.resolveDependancies("%s/%s/%s" % (self.pluginDir, pluginSubDir, pluginName))

    def installQtConf(self):
        qtConfName = self.appDir + "/qt.conf"
        qtConfContent = QT_CONFIG_NOBUNDLE
        if self.bundle:
            qtConfContent = QT_CONFIG
            qtConfName = self.appDir + "/Resources/qt.conf"

        qtConf = open(qtConfName, 'w')
        qtConf.write(qtConfContent)
        qtConf.close()

    def resolveDependancies(self, obj):
        # obj must be either an application binary or a framework library
        # print "resolving deps for:", obj
        for framework, lib in self.determineDependancies(obj):
            self.installFramework(framework)
            self.changeDylPath(obj, framework, lib)

    def installFramework(self, framework):
        # skip if framework is already installed.
        if framework in self.installedFrameworks:
            return

        self.installedFrameworks.add(framework)

        # ensure that the framework directory exists
        try:
            os.mkdir(self.frameworkDir)
        except:
            pass

        if not framework.startswith('/'):
            framework = "%s/%s" % (self.sourceFrameworkPath, framework)

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
            if otoolProcess.wait() == 1:  # we found some Resource dir or similar -> skip
                continue
            frameworkpath, libpath = libname.split(frameworkname)
            if self.bundle:
                newlibname = "@executable_path/../%s%s" % (frameworkname, libpath)
            else:
                newlibname = "@executable_path/%s%s" % (frameworkname, libpath)
            # print 'install_name_tool -id "%s" "%s"' % (newlibname, lib)
            os.system('chmod +w "%s"' % (lib))
            os.system('install_name_tool -id "%s" "%s"' % (newlibname, lib))

            self.resolveDependancies(lib)

    def determineDependancies(self, app):
        otoolPipe = Popen('otool -L "%s"' % app, shell=True, stdout=PIPE).stdout
        otoolOutput = [line for line in otoolPipe]
        otoolPipe.close()
        libs = [line.split()[0] for line in otoolOutput[1:] if ("Qt" in line or "phonon" in line) and "@executable_path" not in line]
        frameworks = [lib[:lib.find(".framework") + len(".framework")] for lib in libs]
        frameworks = [framework[framework.rfind('/') + 1:] for framework in frameworks]
        return zip(frameworks, libs)

    def changeDylPath(self, obj, framework, lib):
        newlibname = framework + lib.split(framework)[1]
        if self.bundle:
            newlibname = "@executable_path/../Frameworks/%s" % newlibname
        else:
            newlibname = "@executable_path/Frameworks/%s" % newlibname

        # print 'install_name_tool -change "%s" "%s" "%s"' % (lib, newlibname, obj)
        os.system('chmod +w "%s"' % (lib))
        os.system('chmod +w "%s"' % (obj))
        os.system('install_name_tool -change "%s" "%s" "%s"' % (lib, newlibname, obj))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Wrong Argument Count (Syntax: %s [--nobundle] [--plugins=plugin1,plugin2,...] $TARGET_APP)" % sys.argv[0]
        sys.exit(1)
    else:
        bundle = True
        plugins = []
        offset = 1

        while offset < len(sys.argv) and sys.argv[offset].startswith("--"):
            if sys.argv[offset] == "--nobundle":
                bundle = False

            if sys.argv[offset].startswith("--plugins="):
                plugins = sys.argv[offset].split('=')[1].split(',')

            offset += 1

        targetDir = sys.argv[offset]
        if bundle:
            targetDir += "/Contents"

        InstallQt(targetDir, bundle, plugins)

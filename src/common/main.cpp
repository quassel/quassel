/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <cstdlib>

#include <QTextCodec>

#ifdef BUILD_CORE
#  include "coreapplication.h"
#elif defined BUILD_QTUI
#  include "aboutdata.h"
#  include "qtuiapplication.h"
#elif defined BUILD_MONO
#  include "aboutdata.h"
#  include "monoapplication.h"

#else
#error "Something is wrong - you need to #define a build mode!"
#endif

// We don't want quasselcore to depend on KDE
#if defined HAVE_KDE4 && defined BUILD_CORE
#  undef HAVE_KDE4
#endif
// We don't want quasselcore to depend on KDE
#if defined HAVE_KF5 && defined BUILD_CORE
#  undef HAVE_KF5
#endif

#ifdef HAVE_KDE4
#  include <KAboutData>
#  include "kcmdlinewrapper.h"
#elif defined HAVE_KF5
#  include <KCoreAddons/KAboutData>
#  include <KCoreAddons/Kdelibs4ConfigMigrator>
#  include "qt5cliparser.h"
#elif defined HAVE_QT5
#  include "qt5cliparser.h"
#else
#  include "cliparser.h"
#endif

#if !defined(BUILD_CORE) && defined(STATIC)
#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qgif)
#endif

#include "quassel.h"

int main(int argc, char **argv)
{
#if QT_VERSION < 0x050000
    // All our source files are in UTF-8, and Qt5 even requires that
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif

    Quassel::setupBuildInfo();
    QCoreApplication::setApplicationName(Quassel::buildInfo().applicationName);
    QCoreApplication::setApplicationVersion(Quassel::buildInfo().plainVersionString);
    QCoreApplication::setOrganizationName(Quassel::buildInfo().organizationName);
    QCoreApplication::setOrganizationDomain(Quassel::buildInfo().organizationDomain);

    // on OSX with Qt4, raster seems to fix performance issues
#if QT_VERSION < 0x050000 && defined Q_OS_MAC && !defined BUILD_CORE
    QApplication::setGraphicsSystem("raster");
#endif

    // We need to explicitly initialize the required resources when linking statically
#ifndef BUILD_QTUI
    Q_INIT_RESOURCE(sql);
#endif
#ifndef BUILD_CORE
    Q_INIT_RESOURCE(pics);
    Q_INIT_RESOURCE(hicolor);
#endif

#ifdef EMBED_DATA
    Q_INIT_RESOURCE(i18n);
# ifndef BUILD_CORE
    Q_INIT_RESOURCE(data);
#   ifdef WITH_OXYGEN
    Q_INIT_RESOURCE(oxygen);
#   endif
#   ifdef WITH_BREEZE
    Q_INIT_RESOURCE(breeze);
#   endif
#   ifdef WITH_BREEZE_DARK
    Q_INIT_RESOURCE(breezedark);
#   endif
# endif
#endif

    AbstractCliParser *cliParser;

#ifdef HAVE_KDE4
    // We need to init KCmdLineArgs first
    KAboutData aboutData("quassel", "kdelibs4", ki18n("Quassel IRC"), Quassel::buildInfo().plainVersionString.toUtf8(),
        ki18n("A modern, distributed IRC client"));
    aboutData.addLicense(KAboutData::License_GPL_V2);
    aboutData.addLicense(KAboutData::License_GPL_V3);
    aboutData.setBugAddress("http://bugs.quassel-irc.org/projects/quassel-irc/issues/new");
    aboutData.setOrganizationDomain(Quassel::buildInfo().organizationDomain.toUtf8());
    KCmdLineArgs::init(argc, argv, &aboutData);

    cliParser = new KCmdLineWrapper();
#elif defined HAVE_QT5
    cliParser = new Qt5CliParser();
#else
    cliParser = new CliParser();
#endif
    Quassel::setCliParser(cliParser);

    // Initialize CLI arguments
    // NOTE: We can't use tr() at this point, since app is not yet created
    // TODO: Change this once we get rid of KDE4 and can initialize the parser after creating the app

    // put shared client&core arguments here
    cliParser->addSwitch("debug", 'd', "Enable debug output");
    cliParser->addSwitch("help", 'h', "Display this help and exit");
    cliParser->addSwitch("version", 'v', "Display version information");
#ifdef BUILD_QTUI
    cliParser->addOption("configdir", 'c', "Specify the directory holding the client configuration", "path");
#else
    cliParser->addOption("configdir", 'c', "Specify the directory holding configuration files, the SQlite database and the SSL certificate", "path");
#endif
    cliParser->addOption("datadir", 0, "DEPRECATED - Use --configdir instead", "path");

#ifndef BUILD_CORE
    // put client-only arguments here
    cliParser->addOption("icontheme", 0, "Override the system icon theme ('oxygen' is recommended)", "theme");
    cliParser->addOption("qss", 0, "Load a custom application stylesheet", "file.qss");
    cliParser->addSwitch("debugbufferswitches", 0, "Enables debugging for bufferswitches");
    cliParser->addSwitch("debugmodel", 0, "Enables debugging for models");
    cliParser->addSwitch("hidewindow", 0, "Start the client minimized to the system tray");
#endif
#ifndef BUILD_QTUI
    // put core-only arguments here
    cliParser->addOption("listen", 0, "The address(es) quasselcore will listen on", "<address>[,<address>[,...]]", "::,0.0.0.0");
    cliParser->addOption("port", 'p', "The port quasselcore will listen at", "port", "4242");
    cliParser->addSwitch("norestore", 'n', "Don't restore last core's state");
    cliParser->addOption("loglevel", 'L', "Loglevel Debug|Info|Warning|Error", "level", "Info");
#ifdef HAVE_SYSLOG
    cliParser->addSwitch("syslog", 0, "Log to syslog");
#endif
    cliParser->addOption("logfile", 'l', "Log to a file", "path");
    cliParser->addOption("select-backend", 0, "Switch storage backend (migrating data if possible)", "backendidentifier");
    cliParser->addSwitch("add-user", 0, "Starts an interactive session to add a new core user");
    cliParser->addOption("change-userpass", 0, "Starts an interactive session to change the password of the user identified by <username>", "username");
    cliParser->addSwitch("oidentd", 0, "Enable oidentd integration");
    cliParser->addOption("oidentd-conffile", 0, "Set path to oidentd configuration file", "file");
#ifdef HAVE_SSL
    cliParser->addSwitch("require-ssl", 0, "Require SSL for remote (non-loopback) client connections");
    cliParser->addOption("ssl-cert", 0, "Specify the path to the SSL Certificate", "path", "configdir/quasselCert.pem");
    cliParser->addOption("ssl-key", 0, "Specify the path to the SSL key", "path", "ssl-cert-path");
#endif
    cliParser->addSwitch("enable-experimental-dcc", 0, "Enable highly experimental and unfinished support for CTCP DCC (DANGEROUS)");
#endif

#ifdef HAVE_KDE4
    // the KDE version needs this extra call to parse argc/argv before app is instantiated
    if (!cliParser->init()) {
        cliParser->usage();
        return EXIT_FAILURE;
    }
#endif

#  if defined BUILD_CORE
    CoreApplication app(argc, argv);
#  elif defined BUILD_QTUI
    QtUiApplication app(argc, argv);
#  elif defined BUILD_MONO
    MonolithicApplication app(argc, argv);
#  endif

#ifndef HAVE_KDE4
    // the non-KDE version parses after app has been instantiated
    if (!cliParser->init(app.arguments())) {
        cliParser->usage();
        return false;
    }
#endif

// Migrate settings from KDE4 to KF5 if appropriate
#ifdef HAVE_KF5
    Kdelibs4ConfigMigrator migrator(QCoreApplication::applicationName());
    migrator.setConfigFiles(QStringList() << "quasselrc" << "quassel.notifyrc");
    migrator.migrate();
#endif

#ifdef HAVE_KF5
    // FIXME: This should be done after loading the translation catalogue, but still in main()
    AboutData aboutData;
    AboutData::setQuasselPersons(&aboutData);
    KAboutData::setApplicationData(aboutData.kAboutData());
#endif

    if (!app.init())
        return EXIT_FAILURE;

    return app.exec();
}

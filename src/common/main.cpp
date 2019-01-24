/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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
#include <memory>

#ifdef HAVE_UMASK
#  include <sys/types.h>
#  include <sys/stat.h>
#endif /* HAVE_UMASK */

#include <QCoreApplication>
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
#include "types.h"

int main(int argc, char **argv)
{
#ifdef HAVE_UMASK
    umask(S_IRWXG | S_IRWXO);
#endif

    // Instantiate early, so log messages are handled
    Quassel quassel;

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
//Setup the High-DPI settings
# if QT_VERSION >= 0x050600 && defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling); //Added in Qt 5.6
#endif
# if QT_VERSION >= 0x050400
   //Added in the early Qt5 versions (5.0?)- use 5.4 as the cutoff since lots of high-DPI work was added then
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
# endif
    // We need to explicitly initialize the required resources when linking statically
#ifndef BUILD_QTUI
    Q_INIT_RESOURCE(sql);
#endif
#ifndef BUILD_CORE
    Q_INIT_RESOURCE(pics);
    Q_INIT_RESOURCE(hicolor_icons);
#endif

#ifdef EMBED_DATA
    Q_INIT_RESOURCE(i18n);
# ifndef BUILD_CORE
    Q_INIT_RESOURCE(data);
    Q_INIT_RESOURCE(breeze_icons);
    Q_INIT_RESOURCE(breeze_dark_icons);
#  ifdef WITH_OXYGEN_ICONS
    Q_INIT_RESOURCE(oxygen_icons);
#  endif
#  ifdef WITH_BUNDLED_ICONS
      Q_INIT_RESOURCE(breeze_icon_theme);
      Q_INIT_RESOURCE(breeze_dark_icon_theme);
#   ifdef WITH_OXYGEN_ICONS
      Q_INIT_RESOURCE(oxygen_icon_theme);
#   endif
#  endif
# endif
#endif

    std::shared_ptr<AbstractCliParser> cliParser;

#ifdef HAVE_KDE4
    // We need to init KCmdLineArgs first
    KAboutData aboutData("quassel", "kdelibs4", ki18n("Quassel IRC"), Quassel::buildInfo().plainVersionString.toUtf8(),
        ki18n("A modern, distributed IRC client"));
    aboutData.addLicense(KAboutData::License_GPL_V2);
    aboutData.addLicense(KAboutData::License_GPL_V3);
    aboutData.setBugAddress("https://bugs.quassel-irc.org/projects/quassel-irc/issues/new");
    aboutData.setOrganizationDomain(Quassel::buildInfo().organizationDomain.toUtf8());
    KCmdLineArgs::init(argc, argv, &aboutData);

    cliParser = std::make_shared<KCmdLineWrapper>();
#elif defined HAVE_QT5
    cliParser = std::make_shared<Qt5CliParser>();
#else
    cliParser = std::make_shared<CliParser>();
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
    cliParser->addOption("loglevel", 'L', "Loglevel Debug|Info|Warning|Error", "level", "Info");
#ifdef HAVE_SYSLOG
    cliParser->addSwitch("syslog", 0, "Log to syslog");
#endif
    cliParser->addOption("logfile", 'l', "Log to a file", "path");

#ifndef BUILD_CORE
    // put client-only arguments here
    cliParser->addOption("icontheme", 0, "Override the system icon theme ('breeze' is recommended)", "theme");
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
    cliParser->addSwitch("config-from-environment", 0, "Load configuration from environment variables");
    cliParser->addOption("select-backend", 0, "Switch storage backend (migrating data if possible)", "backendidentifier");
    cliParser->addOption("select-authenticator", 0, "Select authentication backend", "authidentifier");
    cliParser->addSwitch("add-user", 0, "Starts an interactive session to add a new core user");
    cliParser->addOption("change-userpass", 0, "Starts an interactive session to change the password of the user identified by <username>", "username");
    cliParser->addSwitch("oidentd", 0, "Enable oidentd integration.  In most cases you should also enable --strict-ident");
    cliParser->addOption("oidentd-conffile", 0, "Set path to oidentd configuration file", "file");
    cliParser->addSwitch("strict-ident", 0, "Use users' quasselcore username as ident reply. Ignores each user's configured ident setting.");
    cliParser->addSwitch("ident-daemon", 0, "Enable internal ident daemon");
    cliParser->addOption("ident-port", 0, "The port quasselcore will listen at for ident requests. Only meaningful with --ident-daemon", "port", "10113");
#ifdef HAVE_SSL
    cliParser->addSwitch("require-ssl", 0, "Require SSL for remote (non-loopback) client connections");
    cliParser->addOption("ssl-cert", 0, "Specify the path to the SSL Certificate", "path", "configdir/quasselCert.pem");
    cliParser->addOption("ssl-key", 0, "Specify the path to the SSL key", "path", "ssl-cert-path");
#endif
    cliParser->addSwitch("debug-irc", 0,
                         "Enable logging of all raw IRC messages to debug log, including "
                         "passwords!  In most cases you should also set --loglevel Debug");
    cliParser->addOption("debug-irc-id", 0,
                         "Limit raw IRC logging to this network ID.  Implies --debug-irc",
                         "database network ID", "-1");
    cliParser->addSwitch("enable-experimental-dcc", 0, "Enable highly experimental and unfinished support for CTCP DCC (DANGEROUS)");
#endif

#ifdef HAVE_KDE4
    // the KDE version needs this extra call to parse argc/argv before app is instantiated
    if (!cliParser->init()) {
        cliParser->usage();
        return EXIT_FAILURE;
    }
#endif

#if defined BUILD_CORE
    CoreApplication app(argc, argv);
#elif defined BUILD_QTUI
    QtUiApplication app(argc, argv);
#elif defined BUILD_MONO
    MonolithicApplication app(argc, argv);
#endif

#ifndef HAVE_KDE4
    // the non-KDE version parses after app has been instantiated
    if (!cliParser->init(app.arguments())) {
        cliParser->usage();
        return EXIT_FAILURE;
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
    try {
        app.init();
    }
    catch (ExitException e) {
        if (!e.errorString.isEmpty()) {
            qCritical() << e.errorString;
        }
        return e.exitCode;
    }

    return app.exec();
}

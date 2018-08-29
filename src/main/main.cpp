/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
#if defined HAVE_KF5 && defined BUILD_CORE
#  undef HAVE_KF5
#endif

#if defined HAVE_KF5
#  include <KCoreAddons/KAboutData>
#  include <KCoreAddons/Kdelibs4ConfigMigrator>
#endif

#include "quassel.h"
#include "types.h"

int main(int argc, char **argv)
{
    // Set umask so files are created with restricted permissions
#ifdef HAVE_UMASK
    umask(S_IRWXG | S_IRWXO);
#endif

    // Instantiate early, so log messages are handled
    Quassel quassel;

    Quassel::setupBuildInfo();
    QCoreApplication::setApplicationName(Quassel::buildInfo().applicationName);
    QCoreApplication::setApplicationVersion(Quassel::buildInfo().plainVersionString);
    QCoreApplication::setOrganizationName(Quassel::buildInfo().organizationName);
    QCoreApplication::setOrganizationDomain(Quassel::buildInfo().organizationDomain);

    // Migrate settings from KDE4 to KF5 if appropriate
#ifdef HAVE_KF5
    Kdelibs4ConfigMigrator migrator(QCoreApplication::applicationName());
    migrator.setConfigFiles(QStringList() << "quasselrc" << "quassel.notifyrc");
    migrator.migrate();
#endif

    //Setup the High-DPI settings
# if QT_VERSION >= 0x050600 && defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling); //Added in Qt 5.6
#endif
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Instantiate application
#if defined BUILD_CORE
    CoreApplication app(argc, argv);
    const auto runMode = Quassel::RunMode::CoreOnly;
#elif defined BUILD_QTUI
    QtUiApplication app(argc, argv);
    const auto runMode = Quassel::RunMode::ClientOnly;
#elif defined BUILD_MONO
    MonolithicApplication app(argc, argv);
    const auto runMode = Quassel::RunMode::Monolithic;
#endif

    try {
        Quassel::instance()->init(runMode);

#ifdef HAVE_KF5
        AboutData aboutData;
        AboutData::setQuasselPersons(&aboutData);
        KAboutData::setApplicationData(aboutData.kAboutData());
#endif

        // Initialize the application
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

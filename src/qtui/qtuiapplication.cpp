/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "qtuiapplication.h"

#include <QStringList>

#ifdef HAVE_KDE
#  include <KStandardDirs>
#endif

#include "client.h"
#include "cliparser.h"
#include "mainwin.h"
#include "qtui.h"
#include "qtuisettings.h"

QtUiApplication::QtUiApplication(int &argc, char **argv)
#ifdef HAVE_KDE
    : KApplication(),
#else
    : QApplication(argc, argv),
#endif
    Quassel(),
    _aboutToQuit(false)
{
#ifdef HAVE_KDE
    Q_UNUSED(argc); Q_UNUSED(argv);

    // We need to setup KDE's data dirs
    QStringList dataDirs = KGlobal::dirs()->findDirs("data", "");
    for (int i = 0; i < dataDirs.count(); i++)
        dataDirs[i].append("quassel/");
    dataDirs.append(":/data/");
    setDataDirPaths(dataDirs);

#else /* HAVE_KDE */

    setDataDirPaths(findDataDirPaths());

#endif /* HAVE_KDE */

#if defined(HAVE_KDE) || defined(Q_OS_MAC)
    disableCrashhandler();
#endif /* HAVE_KDE || Q_OS_MAC */
    setRunMode(Quassel::ClientOnly);

    qInstallMsgHandler(Client::logMessage);
}


bool QtUiApplication::init()
{
    if (Quassel::init()) {
        // FIXME: MIGRATION 0.3 -> 0.4: Move database and core config to new location
        // Move settings, note this does not delete the old files
#ifdef Q_WS_MAC
        QSettings newSettings("quassel-irc.org", "quasselclient");
#else

# ifdef Q_WS_WIN
        QSettings::Format format = QSettings::IniFormat;
# else
        QSettings::Format format = QSettings::NativeFormat;
# endif

        QString newFilePath = Quassel::configDirPath() + "quasselclient"
                              + ((format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
        QSettings newSettings(newFilePath, format);
#endif /* Q_WS_MAC */

        if (newSettings.value("Config/Version").toUInt() == 0) {
#     ifdef Q_WS_MAC
            QString org = "quassel-irc.org";
#     else
            QString org = "Quassel Project";
#     endif
            QSettings oldSettings(org, "Quassel Client");
            if (oldSettings.allKeys().count()) {
                qWarning() << "\n\n*** IMPORTANT: Config and data file locations have changed. Attempting to auto-migrate your client settings...";
                foreach(QString key, oldSettings.allKeys())
                newSettings.setValue(key, oldSettings.value(key));
                newSettings.setValue("Config/Version", 1);
                qWarning() << "*   Your client settings have been migrated to" << newSettings.fileName();
                qWarning() << "*** Migration completed.\n\n";
            }
        }

        // MIGRATION end

        // check settings version
        // so far, we only have 1
        QtUiSettings s;
        if (s.version() != 1) {
            qCritical() << "Invalid client settings version, terminating!";
            return false;
        }

        // session resume
        QtUi *gui = new QtUi();
        Client::init(gui);
        // init gui only after the event loop has started
        // QTimer::singleShot(0, gui, SLOT(init()));
        gui->init();
        resumeSessionIfPossible();
        return true;
    }
    return false;
}


QtUiApplication::~QtUiApplication()
{
    Client::destroy();
}


void QtUiApplication::quit()
{
    QtUi::mainWindow()->quit();
}


void QtUiApplication::commitData(QSessionManager &manager)
{
    Q_UNUSED(manager)
    _aboutToQuit = true;
}


void QtUiApplication::saveState(QSessionManager &manager)
{
    //qDebug() << QString("saving session state to id %1").arg(manager.sessionId());
    // AccountId activeCore = Client::currentCoreAccount().accountId(); // FIXME store this!
    SessionSettings s(manager.sessionId());
    s.setSessionAge(0);
    QtUi::mainWindow()->saveStateToSettings(s);
}


void QtUiApplication::resumeSessionIfPossible()
{
    // load all sessions
    if (isSessionRestored()) {
        qDebug() << QString("restoring from session %1").arg(sessionId());
        SessionSettings s(sessionId());
        s.sessionAging();
        s.setSessionAge(0);
        QtUi::mainWindow()->restoreStateFromSettings(s);
        s.cleanup();
    }
    else {
        SessionSettings s(QString("1"));
        s.sessionAging();
        s.cleanup();
    }
}

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

#include "qtuiapplication.h"

#include <QIcon>
#include <QStringList>

#ifdef HAVE_KDE4
#  include <KStandardDirs>
#endif

#include "client.h"
#include "cliparser.h"
#include "mainwin.h"
#include "qtui.h"
#include "qtuisettings.h"

QtUiApplication::QtUiApplication(int &argc, char **argv)
#ifdef HAVE_KDE4
    : KApplication(),  // KApplication is deprecated in KF5
#else
    : QApplication(argc, argv),
#endif
    Quassel(),
    _aboutToQuit(false)
{
#ifdef HAVE_KDE4
    Q_UNUSED(argc); Q_UNUSED(argv);

    // Setup KDE's data dirs
    // Because we can't use KDE stuff in (the class) Quassel directly, we need to do this here...
    QStringList dataDirs = KGlobal::dirs()->findDirs("data", "");

    // Just in case, also check our install prefix
    dataDirs << QCoreApplication::applicationDirPath() + "/../share/apps/";

    // Normalize and append our application name
    for (int i = 0; i < dataDirs.count(); i++)
        dataDirs[i] = QDir::cleanPath(dataDirs.at(i)) + "/quassel/";

    // Add resource path and just in case.
    // Workdir should have precedence
    dataDirs.prepend(QCoreApplication::applicationDirPath() + "/data/");
    dataDirs.append(":/data/");

    // Append trailing '/' and check for existence
    auto iter = dataDirs.begin();
    while (iter != dataDirs.end()) {
        if (!iter->endsWith(QDir::separator()) && !iter->endsWith('/'))
            iter->append(QDir::separator());
        if (!QFile::exists(*iter))
            iter = dataDirs.erase(iter);
        else
            ++iter;
    }

    dataDirs.removeDuplicates();
    setDataDirPaths(dataDirs);

#else /* HAVE_KDE4 */

    setDataDirPaths(findDataDirPaths());

#endif /* HAVE_KDE4 */

#if defined(HAVE_KDE4) || defined(Q_OS_MAC)
    disableCrashhandler();
#endif /* HAVE_KDE4 || Q_OS_MAC */
    setRunMode(Quassel::ClientOnly);

#if QT_VERSION < 0x050000
    qInstallMsgHandler(Client::logMessage);
#else
    qInstallMessageHandler(Client::logMessage);
    connect(this, &QGuiApplication::commitDataRequest, this, &QtUiApplication::commitData, Qt::DirectConnection);
    connect(this, &QGuiApplication::saveStateRequest, this, &QtUiApplication::saveState, Qt::DirectConnection);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QGuiApplication::setFallbackSessionManagementEnabled(false);
#endif
}


bool QtUiApplication::init()
{
    if (Quassel::init()) {
        // FIXME: MIGRATION 0.3 -> 0.4: Move database and core config to new location
        // Move settings, note this does not delete the old files
#ifdef Q_OS_MAC
        QSettings newSettings("quassel-irc.org", "quasselclient");
#else

# ifdef Q_OS_WIN
        QSettings::Format format = QSettings::IniFormat;
# else
        QSettings::Format format = QSettings::NativeFormat;
# endif

        QString newFilePath = Quassel::configDirPath() + "quasselclient"
                              + ((format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
        QSettings newSettings(newFilePath, format);
#endif /* Q_OS_MAC */

        if (newSettings.value("Config/Version").toUInt() == 0) {
#     ifdef Q_OS_MAC
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

        // Checking if settings Icon Theme is valid
        QString savedIcontheme = QtUiSettings().value("IconTheme", QVariant("")).toString();
#ifndef WITH_OXYGEN
        if (savedIcontheme == "oxygen")
            QtUiSettings().remove("IconTheme");
#endif
#ifndef WITH_BREEZE
        if (savedIcontheme == "breeze")
            QtUiSettings().remove("IconTheme");
#endif
#ifndef WITH_BREEZE_DARK
        if (savedIcontheme == "breezedark")
            QtUiSettings().remove("IconTheme");
#endif

        // Set the icon theme
        if (Quassel::isOptionSet("icontheme"))
            QIcon::setThemeName(Quassel::optionValue("icontheme"));
        else if (QtUiSettings().value("IconTheme", QVariant("")).toString() != "")
            QIcon::setThemeName(QtUiSettings().value("IconTheme").toString());
        else if (QIcon::themeName().isEmpty())
            // Some platforms don't set a default icon theme; chances are we can find our bundled Oxygen theme though
            QIcon::setThemeName("oxygen");

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

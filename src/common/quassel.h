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

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QLocale>
#include <QString>

#include "abstractcliparser.h"

class QFile;

class Quassel
{
    Q_DECLARE_TR_FUNCTIONS(Quassel)

public:
    enum RunMode {
        Monolithic,
        ClientOnly,
        CoreOnly
    };

    struct BuildInfo {
        QString fancyVersionString; // clickable rev
        QString plainVersionString; // no <a> tag

        QString baseVersion;
        QString generatedVersion;
        QString commitHash;
        QString commitDate;

        uint protocolVersion; // deprecated

        QString applicationName;
        QString coreApplicationName;
        QString clientApplicationName;
        QString organizationName;
        QString organizationDomain;
    };

    //! A list of features that are optional in core and/or client, but need runtime checking
    /** Some features require an uptodate counterpart, but don't justify a protocol break.
     *  This is what we use this enum for. Add such features to it and check at runtime on the other
     *  side for their existence.
     *
     *  This list should be cleaned up after every protocol break, as we can assume them to be present then.
     */
    enum Feature {
        SynchronizedMarkerLine = 0x0001,
        SaslAuthentication = 0x0002,
        SaslExternal = 0x0004,
        HideInactiveNetworks = 0x0008,
        PasswordChange = 0x0010,
        CapNegotiation = 0x0020,           /// IRCv3 capability negotiation, account tracking
        VerifyServerSSL = 0x0040,          /// IRC server SSL validation
        CustomRateLimits = 0x0080,         /// IRC server custom message rate limits
        DccFileTransfer = 0x0100,          /// DCC file transfer support (forcefully disabled for now)
        AwayFormatTimestamp = 0x0200,      /// Timestamp formatting in away (e.g. %%hh:mm%%)
        Authenticators = 0x0400,           /// Whether or not the core supports auth backends.
        BufferActivitySync = 0x0800,       /// Sync buffer activity status
        CoreSideHighlights = 0x1000,       /// Core-Side highlight configuration and matching
        SenderPrefixes = 0x2000,           /// Show prefixes for senders in backlog
        RemoteDisconnect = 0x4000,         /// Allow this peer to be remotely disconnected

        NumFeatures = 0x4000
    };
    Q_DECLARE_FLAGS(Features, Feature)

    //! The features the current version of Quassel supports (\sa Feature)
    /** \return An ORed list of all enum values in Feature
     */
    static Features features();

    static Quassel *instance();

    static void setupBuildInfo();
    static const BuildInfo &buildInfo();
    static RunMode runMode();

    static QString configDirPath();

    //! Returns a list of data directory paths
    /** There are several locations for applications to install their data files in. On Unix,
    *  a common location is /usr/share; others include $PREFIX/share and additional directories
    *  specified in the env variable XDG_DATA_DIRS.
    *  \return A list of directory paths to look for data files in
    */
    static QStringList dataDirPaths();

    //! Searches for a data file in the possible data directories
    /** Data files can reside in $DATA_DIR/apps/quassel, where $DATA_DIR is one of the directories
    *  returned by \sa dataDirPaths().
    *  \Note With KDE integration enabled, files are searched (only) in KDE's appdata dirs.
    *  \return The full path to the data file if found; a null QString else
    */
    static QString findDataFilePath(const QString &filename);

    static QString translationDirPath();

    //! Returns a list of directories we look for scripts in
    /** We look for a subdirectory named "scripts" in the configdir and in all datadir paths.
    *   \return A list of directory paths containing executable scripts for /exec
    */
    static QStringList scriptDirPaths();

    static void loadTranslation(const QLocale &locale);

    static void setCliParser(std::shared_ptr<AbstractCliParser> cliParser);
    static QString optionValue(const QString &option);
    static bool isOptionSet(const QString &option);

    enum LogLevel {
        DebugLevel,
        InfoLevel,
        WarningLevel,
        ErrorLevel
    };

    static LogLevel logLevel();
    static void setLogLevel(LogLevel logLevel);
    static QFile *logFile();
    static bool logToSyslog();

    static void logFatalMessage(const char *msg);

    using ReloadHandler = std::function<bool()>;

    static void registerReloadHandler(ReloadHandler handler);

    using QuitHandler = std::function<void()>;

    static void registerQuitHandler(QuitHandler quitHandler);

protected:
    static bool init();
    static void destroy();

    static void setRunMode(Quassel::RunMode runMode);

    static void setDataDirPaths(const QStringList &paths);
    static QStringList findDataDirPaths();
    static void disableCrashHandler();

    friend class CoreApplication;
    friend class QtUiApplication;
    friend class MonolithicApplication;

private:
    Quassel();
    void setupEnvironment();
    void registerMetaTypes();

    const QString &coreDumpFileName();

    /**
     * Requests a reload of relevant runtime configuration.
     *
     * Calls any registered reload handlers, and returns the cumulative result. If no handlers are registered,
     * does nothing and returns true.
     *
     * @returns True if configuration reload successful, otherwise false
     */
    bool reloadConfig();

    /**
     * Requests to quit the application.
     *
     * Calls any registered quit handlers. If no handlers are registered, calls QCoreApplication::quit().
     */
    void quit();

    void logBacktrace(const QString &filename);

    static void handleSignal(int signal);

private:
    BuildInfo _buildInfo;
    RunMode _runMode;
    bool _initialized{false};
    bool _handleCrashes{true};

    QString _coreDumpFileName;
    QString _configDirPath;
    QStringList _dataDirPaths;
    QString _translationDirPath;

    LogLevel _logLevel{InfoLevel};
    bool _logToSyslog{false};
    std::unique_ptr<QFile> _logFile;

    std::shared_ptr<AbstractCliParser> _cliParser;

    std::vector<ReloadHandler> _reloadHandlers;
    std::vector<QuitHandler> _quitHandlers;
};


Q_DECLARE_OPERATORS_FOR_FLAGS(Quassel::Features);

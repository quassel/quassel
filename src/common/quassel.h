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

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QFile>
#include <QObject>
#include <QLocale>
#include <QString>
#include <QStringList>

#include "abstractcliparser.h"

class QFile;

class Quassel : public QObject
{
    // TODO Qt5: Use Q_GADGET
    Q_OBJECT

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

    /**
     * This enum defines the optional features supported by cores/clients prior to version 0.13.
     *
     * Since the number of features declared this way is limited to 16 (due to the enum not having a defined
     * width in cores/clients prior to 0.13), and for more robustness when negotiating features on connect,
     * the bitfield-based representation was replaced by a string-based representation in 0.13, support for
     * which is indicated by having the ExtendedFeatures flag set. Extended features are defined in the Feature
     * enum.
     *
     * @warning Do not alter this enum; new features must be added (only) to the @a Feature enum.
     *
     * @sa Feature
     */
    enum class LegacyFeature : quint32 {
        SynchronizedMarkerLine = 0x0001,
        SaslAuthentication     = 0x0002,
        SaslExternal           = 0x0004,
        HideInactiveNetworks   = 0x0008,
        PasswordChange         = 0x0010,
        CapNegotiation         = 0x0020,
        VerifyServerSSL        = 0x0040,
        CustomRateLimits       = 0x0080,
        // DccFileTransfer     = 0x0100,  // never in use
        AwayFormatTimestamp    = 0x0200,
        Authenticators         = 0x0400,
        BufferActivitySync     = 0x0800,
        CoreSideHighlights     = 0x1000,
        SenderPrefixes         = 0x2000,
        RemoteDisconnect       = 0x4000,
        ExtendedFeatures       = 0x8000,
    };
    Q_FLAGS(LegacyFeature)
    Q_DECLARE_FLAGS(LegacyFeatures, LegacyFeature)

    /**
     * A list of features that are optional in core and/or client, but need runtime checking.
     *
     * Some features require an uptodate counterpart, but don't justify a protocol break.
     * This is what we use this enum for. Add such features to it and check at runtime on the other
     * side for their existence.
     *
     * For feature negotiation, these enum values are serialized as strings, so order does not matter. However,
     * do not rename existing enum values to avoid breaking compatibility.
     *
     * This list should be cleaned up after every protocol break, as we can assume them to be present then.
     */
    #if QT_VERSION >= 0x050000
    enum class Feature : uint32_t {
    #else
    enum Feature {
    #endif
        SynchronizedMarkerLine,
        SaslAuthentication,
        SaslExternal,
        HideInactiveNetworks,
        PasswordChange,           ///< Remote password change
        CapNegotiation,           ///< IRCv3 capability negotiation, account tracking
        VerifyServerSSL,          ///< IRC server SSL validation
        CustomRateLimits,         ///< IRC server custom message rate limits
        AwayFormatTimestamp,      ///< Timestamp formatting in away (e.g. %%hh:mm%%)
        Authenticators,           ///< Whether or not the core supports auth backends
        BufferActivitySync,       ///< Sync buffer activity status
        CoreSideHighlights,       ///< Core-Side highlight configuration and matching
        SenderPrefixes,           ///< Show prefixes for senders in backlog
        RemoteDisconnect,         ///< Allow this peer to be remotely disconnected
        ExtendedFeatures,         ///< Extended features
        LongTime,                 ///< Serialize time as 64-bit values
        RichMessages,             ///< Real Name and Avatar URL in backlog
        BacklogFilterType,        ///< BacklogManager supports filtering backlog by MessageType
#if QT_VERSION >= 0x050500
        EcdsaCertfpKeys,          ///< ECDSA keys for CertFP in identities
#endif
        LongMessageId,            ///< 64-bit IDs for messages
    };
    Q_ENUMS(Feature)

    class Features;

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

// --------------------------------------------------------------------------------------------------------------------

/**
 * Class representing a set of supported core/client features.
 *
 * @sa Quassel::Feature
 */
class Quassel::Features
{
public:
    /**
     * Default constructor.
     *
     * Creates a Feature instance with all known features (i.e., all values declared in the Quassel::Feature enum) set.
     * This is useful for easily creating a Feature instance that represent the current version's capabilities.
     */
    Features();

    /**
     * Constructs a Feature instance holding the given list of features.
     *
     * Both the @a features and the @a legacyFeatures arguments are considered (additively).
     * This is useful when receiving a list of features from another peer.
     *
     * @param features       A list of strings matching values in the Quassel::Feature enum. Strings that don't match are
     *                       can be accessed after construction via unknownFeatures(), but are otherwise ignored.
     * @param legacyFeatures Holds a bit-wise combination of LegacyFeature flag values, which are each added to the list of
     *                       features represented by this Features instance.
     */
    Features(const QStringList &features, LegacyFeatures legacyFeatures);

    /**
     * Check if a given feature is marked as enabled in this Features instance.
     *
     * @param feature The feature to be queried
     * @returns Whether the given feature is marked as enabled
     */
    bool isEnabled(Feature feature) const;

    /**
     * Provides a list of all features marked as either enabled or disabled (as indicated by the @a enabled argument) as strings.
     *
     * @param enabled Whether to return the enabled or the disabled features
     * @return A string list containing all enabled or disabled features
     */
    QStringList toStringList(bool enabled = true) const;

    /**
     * Provides a list of all enabled legacy features (i.e. features defined prior to v0.13) as bit-wise combination in a
     * LegacyFeatures type.
     *
     * @note Extended features cannot be represented this way, and are thus ignored even if set.
     * @return A LegacyFeatures type holding the bit-wise combination of all legacy features enabled in this Features instance
     */
    LegacyFeatures toLegacyFeatures() const;

    /**
     * Provides the list of strings that could not be mapped to Quassel::Feature enum values on construction.
     *
     * Useful for debugging/logging purposes.
     *
     * @returns A list of strings that could not be mapped to the Feature enum on construction of this Features instance, if any
     */
    QStringList unknownFeatures() const;

private:
    std::vector<bool> _features;
    QStringList _unknownFeatures;
};

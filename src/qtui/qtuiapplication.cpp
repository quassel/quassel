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

#include "qtuiapplication.h"

#include <QDir>
#include <QFile>
#include <QStringList>

#include "chatviewsettings.h"
#include "mainwin.h"
#include "qtui.h"
#include "qtuisettings.h"
#include "types.h"

QtUiApplication::QtUiApplication(int& argc, char** argv)
    : QApplication(argc, argv)
{
#if QT_VERSION >= 0x050600
    QGuiApplication::setFallbackSessionManagementEnabled(false);
#endif
#if QT_VERSION >= 0x050700
    QGuiApplication::setDesktopFileName(Quassel::buildInfo().clientApplicationName);
#endif
}

void QtUiApplication::init()
{
    // Settings upgrade/downgrade handling
    if (!migrateSettings()) {
        throw ExitException{EXIT_FAILURE, tr("Could not load or upgrade client settings!")};
    }

    _client = std::make_unique<Client>(std::make_unique<QtUi>());

    // Init UI only after the event loop has started
    QTimer::singleShot(0, this, [this]() {
        QtUi::instance()->init();
        connect(this, &QGuiApplication::commitDataRequest, this, &QtUiApplication::commitData, Qt::DirectConnection);
        connect(this, &QGuiApplication::saveStateRequest, this, &QtUiApplication::saveState, Qt::DirectConnection);

        // Needs to happen after UI init, so the MainWin quit handler is registered first
        Quassel::registerQuitHandler(quitHandler());

        resumeSessionIfPossible();
    });
}

Quassel::QuitHandler QtUiApplication::quitHandler()
{
    // Wait until the Client instance is destroyed before quitting the event loop
    return [this]() {
        qInfo() << "Client shutting down...";
        connect(_client.get(), &QObject::destroyed, QCoreApplication::instance(), &QCoreApplication::quit);
        _client.release()->deleteLater();
    };
}

bool QtUiApplication::migrateSettings()
{
    // --------
    // Check major settings version.  This represents incompatible changes between settings
    // versions.  So far, we only have 1.
    QtUiSettings s;
    uint versionMajor = s.version();
    if (versionMajor != 1) {
        qCritical() << qPrintable(QString("Invalid client settings version '%1'").arg(versionMajor));
        return false;
    }

    // --------
    // Check minor settings version, handling upgrades/downgrades as needed
    // Current minor version
    //
    // NOTE:  If you increase the minor version, you MUST ALSO add new version upgrade logic in
    // applySettingsMigration()!  Otherwise, settings upgrades will fail.
    const uint VERSION_MINOR_CURRENT = 9;
    // Stored minor version
    uint versionMinor = s.versionMinor();

    if (versionMinor == VERSION_MINOR_CURRENT) {
        // At latest version, no need to migrate defaults or other settings
        return true;
    }
    else if (versionMinor == 0) {
        // New configuration, store as current version
        qDebug() << qPrintable(QString("Set up new client settings v%1.%2").arg(versionMajor).arg(VERSION_MINOR_CURRENT));
        s.setVersionMinor(VERSION_MINOR_CURRENT);

        // Update the settings stylesheet for first setup.  We don't know if older content exists,
        // if the configuration got erased separately, etc.
        QtUiStyle qtUiStyle;
        qtUiStyle.generateSettingsQss();
        return true;
    }
    else if (versionMinor < VERSION_MINOR_CURRENT) {
        // We're upgrading - apply the neccessary upgrades from each interim version
        // curVersion will never equal VERSION_MINOR_CURRENT, as it represents the version before
        // the most recent applySettingsMigration() call.
        for (uint curVersion = versionMinor; curVersion < VERSION_MINOR_CURRENT; curVersion++) {
            if (!applySettingsMigration(s, curVersion + 1)) {
                // Something went wrong, time to bail out
                qCritical() << qPrintable(QString("Could not migrate client settings from v%1.%2 "
                                                  "to v%1.%3")
                                              .arg(versionMajor)
                                              .arg(curVersion)
                                              .arg(curVersion + 1));
                // Keep track of the last successful upgrade to avoid repeating it on next start
                s.setVersionMinor(curVersion);
                return false;
            }
        }
        // Migration successful!
        qDebug() << qPrintable(QString("Successfully migrated client settings from v%1.%2 to "
                                       "v%1.%3")
                                   .arg(versionMajor)
                                   .arg(versionMinor)
                                   .arg(VERSION_MINOR_CURRENT));
        // Store the new minor version
        s.setVersionMinor(VERSION_MINOR_CURRENT);
        return true;
    }
    else {
        // versionMinor > VERSION_MINOR_CURRENT
        // The user downgraded to an older version of Quassel.  Let's hope for the best.
        // Don't change the minorVersion as the newer version's upgrade logic has already run.
        qWarning() << qPrintable(QString("Client settings v%1.%2 is newer than latest known v%1.%3,"
                                         " things might not work!")
                                     .arg(versionMajor)
                                     .arg(versionMinor)
                                     .arg(VERSION_MINOR_CURRENT));
        return true;
    }
}

bool QtUiApplication::applySettingsMigration(QtUiSettings settings, const uint newVersion)
{
    switch (newVersion) {
    // Version 0 and 1 aren't valid upgrade paths - one represents no version, the other is the
    // oldest version.  Ignore those, start from 2 and higher.
    // Each missed version will be called in sequence.  E.g. to upgrade from '1' to '3', this
    // function will be called with '2', then '3'.
    // Use explicit scope via { ... } to avoid cross-initialization
    //
    // In most cases, the goal is to preserve the older default values for keys that haven't been
    // saved.  Exceptions will be noted below.
    // NOTE:  If you add new upgrade logic here, you MUST ALSO increase VERSION_MINOR_CURRENT in
    // migrateSettings()!  Otherwise, your upgrade logic won't ever be called.
    case 9: {
        // New default changes: show highest sender prefix mode, if available

        // --------
        // ChatView settings
        ChatViewSettings chatViewSettings;
        const QString senderPrefixModeId = "SenderPrefixMode";
        if (!chatViewSettings.valueExists(senderPrefixModeId)) {
            // New default is HighestMode, preserve previous behavior by setting to NoModes
            chatViewSettings.setValue(senderPrefixModeId, static_cast<int>(UiStyle::SenderPrefixMode::NoModes));
        }
        // --------

        // Migration complete!
        return true;
    }

    case 8: {
        // New default changes: RegEx checkbox now toggles Channel regular expressions, too
        //
        // This only affects local highlights.  Core-side highlights weren't released in stable when
        // this change was made, so no need to migrate those.

        // --------
        // NotificationSettings
        NotificationSettings notificationSettings;

        // Check each highlight rule for a "Channel" field.  If one exists, convert to RegEx mode.
        // This might be more efficient with std::transform() or such.  It /is/ only run once...
        auto highlightList = notificationSettings.highlightList();
        bool changesMade = false;
        for (int index = 0; index < highlightList.count(); ++index) {
            // Load the highlight rule...
            auto highlightRule = highlightList[index].toMap();

            // Check if "Channel" has anything set and RegEx is disabled
            if (!highlightRule["Channel"].toString().isEmpty() && highlightRule["RegEx"].toBool() == false) {
                // We have a rule to convert

                // Mark as a regular expression, allowing the Channel filtering to work the same as
                // before the upgrade
                highlightRule["RegEx"] = true;

                // Convert the main rule to regular expression, mirroring the conversion to wildcard
                // format from QtUiMessageProcessor::checkForHighlight()
                highlightRule["Name"] = "(^|\\W)" + QRegExp::escape(highlightRule["Name"].toString()) + "(\\W|$)";

                // Save the rule back
                highlightList[index] = highlightRule;
                changesMade = true;
            }
        }

        // Save the modified rules if any changes were made
        if (changesMade) {
            notificationSettings.setHighlightList(highlightList);
        }
        // --------

        // Migration complete!
        return true;
    }
    case 7: {
        // New default changes: UseProxy is no longer used in CoreAccountSettings
        CoreAccountSettings s;
        for (auto&& accountId : s.knownAccounts()) {
            auto map = s.retrieveAccountData(accountId);
            if (!map.value("UseProxy", false).toBool()) {
                map["ProxyType"] = static_cast<int>(QNetworkProxy::ProxyType::NoProxy);
            }
            map.remove("UseProxy");
            s.storeAccountData(accountId, map);
        }

        // Migration complete!
        return true;
    }
    case 6: {
        // New default changes: sender colors switched around to Tango-ish theme

        // --------
        // QtUiStyle settings
        QtUiStyleSettings settingsUiStyleColors("Colors");
        // Preserve the old default values for all variants
        const QColor oldDefaultSenderColorSelf = QColor(0, 0, 0);
        const QList<QColor> oldDefaultSenderColors = QList<QColor>{
            QColor(204, 13, 127),   /// Sender00
            QColor(142, 85, 233),   /// Sender01
            QColor(179, 14, 14),    /// Sender02
            QColor(23, 179, 57),    /// Sender03
            QColor(88, 175, 179),   /// Sender04
            QColor(157, 84, 179),   /// Sender05
            QColor(179, 151, 117),  /// Sender06
            QColor(49, 118, 179),   /// Sender07
            QColor(233, 13, 127),   /// Sender08
            QColor(142, 85, 233),   /// Sender09
            QColor(179, 14, 14),    /// Sender10
            QColor(23, 179, 57),    /// Sender11
            QColor(88, 175, 179),   /// Sender12
            QColor(157, 84, 179),   /// Sender13
            QColor(179, 151, 117),  /// Sender14
            QColor(49, 118, 179),   /// Sender15
        };
        if (!settingsUiStyleColors.valueExists("SenderSelf")) {
            // Preserve the old default sender color if none set
            settingsUiStyleColors.setValue("SenderSelf", oldDefaultSenderColorSelf);
        }
        QString senderColorId;
        for (int i = 0; i < oldDefaultSenderColors.count(); i++) {
            // Get the sender color ID for each available color
            QString dez = QString::number(i);
            if (dez.length() == 1)
                dez.prepend('0');
            senderColorId = QString("Sender" + dez);
            if (!settingsUiStyleColors.valueExists(senderColorId)) {
                // Preserve the old default sender color if none set
                settingsUiStyleColors.setValue(senderColorId, oldDefaultSenderColors[i]);
            }
        }

        // Update the settings stylesheet with old defaults
        QtUiStyle qtUiStyle;
        qtUiStyle.generateSettingsQss();
        // --------

        // Migration complete!
        return true;
    }
    case 5: {
        // New default changes: sender colors apply to nearly all messages with nicks

        // --------
        // QtUiStyle settings
        QtUiStyleSettings settingsUiStyleColors("Colors");
        const QString useNickGeneralColorsId = "UseNickGeneralColors";
        if (!settingsUiStyleColors.valueExists(useNickGeneralColorsId)) {
            // New default is true, preserve previous behavior by setting to false
            settingsUiStyleColors.setValue(useNickGeneralColorsId, false);
        }

        // Update the settings stylesheet with old defaults
        QtUiStyle qtUiStyle;
        qtUiStyle.generateSettingsQss();
        // --------

        // Migration complete!
        return true;
    }
    case 4: {
        // New default changes: system locale used to generate a timestamp format string, deciding
        // 24-hour or 12-hour timestamp.

        // --------
        // ChatView settings
        const QString useCustomTimestampFormatId = "ChatView/__default__/UseCustomTimestampFormat";
        if (!settings.valueExists(useCustomTimestampFormatId)) {
            // New default value is false, preserve previous behavior by setting to true
            settings.setValue(useCustomTimestampFormatId, true);
        }
        // --------

        // Migration complete!
        return true;
    }
    case 3: {
        // New default changes: per-chat history and line wrapping enabled by default.

        // --------
        // InputWidget settings
        UiSettings settingsInputWidget("InputWidget");
        const QString enableInputPerChatId = "EnablePerChatHistory";
        if (!settingsInputWidget.valueExists(enableInputPerChatId)) {
            // New default value is true, preserve previous behavior by setting to false
            settingsInputWidget.setValue(enableInputPerChatId, false);
        }

        const QString enableInputLinewrap = "EnableLineWrap";
        if (!settingsInputWidget.valueExists(enableInputLinewrap)) {
            // New default value is true, preserve previous behavior by setting to false
            settingsInputWidget.setValue(enableInputLinewrap, false);
        }
        // --------

        // Migration complete!
        return true;
    }
    case 2: {
        // New default changes: sender <nick> brackets disabled, sender colors and sender CTCP
        // colors enabled.

        // --------
        // ChatView settings
        const QString timestampFormatId = "ChatView/__default__/TimestampFormat";
        if (!settings.valueExists(timestampFormatId)) {
            // New default value is " hh:mm:ss", preserve old default of "[hh:mm:ss]"
            settings.setValue(timestampFormatId, "[hh:mm:ss]");
        }

        const QString showSenderBracketsId = "ChatView/__default__/ShowSenderBrackets";
        if (!settings.valueExists(showSenderBracketsId)) {
            // New default is false, preserve previous behavior by setting to true
            settings.setValue(showSenderBracketsId, true);
        }
        // --------

        // --------
        // QtUiStyle settings
        QtUiStyleSettings settingsUiStyleColors("Colors");
        const QString useSenderColorsId = "UseSenderColors";
        if (!settingsUiStyleColors.valueExists(useSenderColorsId)) {
            // New default is true, preserve previous behavior by setting to false
            settingsUiStyleColors.setValue(useSenderColorsId, false);
        }

        const QString useSenderActionColorsId = "UseSenderActionColors";
        if (!settingsUiStyleColors.valueExists(useSenderActionColorsId)) {
            // New default is true, preserve previous behavior by setting to false
            settingsUiStyleColors.setValue(useSenderActionColorsId, false);
        }

        // Update the settings stylesheet with old defaults
        QtUiStyle qtUiStyle;
        qtUiStyle.generateSettingsQss();
        // --------

        // Migration complete!
        return true;
    }
    default:
        // Something unexpected happened
        return false;
    }
}

void QtUiApplication::commitData(QSessionManager& manager)
{
    Q_UNUSED(manager)
    _aboutToQuit = true;
}

void QtUiApplication::saveState(QSessionManager& manager)
{
    // qDebug() << QString("saving session state to id %1").arg(manager.sessionId());
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

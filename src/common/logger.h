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

#include <vector>

#include <QDateTime>
#include <QFile>
#include <QMetaType>
#include <QObject>
#include <QString>

/**
 * The Logger class encapsulates the various configured logging backends.
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    Logger(QObject *parent = nullptr);
    ~Logger() override;

    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    struct LogEntry {
        QDateTime timeStamp;
        LogLevel logLevel;
        QString message;
    };

    /**
     * Initial setup, to be called ones command line options are available.
     *
     * Sets up the log file if appropriate. Outputs the log messages already accumulated since
     * construction. If @c keepMessages is false, deletes the accumulated messages afterwards,
     * and won't store further ones.
     *
     * @param keepMessages Whether messages should be kept
     * @returns true, if initialization was successful
     */
    bool setup(bool keepMessages);

    /**
     * Accesses the stores log messages, e.g. for consumption by DebugLogWidget.
     *
     * @returns The accumuates log messages
     */
    std::vector<Logger::LogEntry> messages() const;

#if QT_VERSION < 0x050000
    static void messageHandler(QtMsgType type, const char *message);
#else
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
#endif

    /**
     * Takes the given message with the given log level, formats it and emits the @a messageLogged() signal.
     *
     * @note This method is thread-safe.
     *
     * @param logLevel The log leve of the message
     * @param message  The message
     */
    void handleMessage(LogLevel logLevel, const QString &message);

signals:
    /**
     * Emitted whenever a message was logged.
     *
     * @param message The message that was logged
     */
    void messageLogged(const Logger::LogEntry &message);

private slots:
    void onMessageLogged(const Logger::LogEntry &message);

private:
    void handleMessage(QtMsgType type, const QString &message);
    void outputMessage(const LogEntry &message);

private:
    LogLevel _outputLevel{LogLevel::Info};
    QFile _logFile;
    bool _syslogEnabled{false};

    std::vector<LogEntry> _messages;
    bool _keepMessages{true};
    bool _initialized{false};
};

Q_DECLARE_METATYPE(Logger::LogEntry)

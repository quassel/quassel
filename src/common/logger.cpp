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

#include <iostream>

#ifdef HAVE_SYSLOG
#  include <syslog.h>
#endif

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>

#include "logger.h"
#include "quassel.h"

namespace {

QByteArray msgWithTime(const Logger::LogEntry &msg)
{
    return (msg.timeStamp.toString("yyyy-MM-dd hh:mm:ss ") + msg.message + "\n").toUtf8();
};

}


Logger::Logger(QObject *parent)
    : QObject(parent)
{
    static bool registered = []() {
        qRegisterMetaType<LogEntry>();
        return true;
    }();
    Q_UNUSED(registered)

    connect(this, SIGNAL(messageLogged(Logger::LogEntry)), this, SLOT(onMessageLogged(Logger::LogEntry)));

#if QT_VERSION < 0x050000
    qInstallMsgHandler(Logger::messageHandler);
#else
    qInstallMessageHandler(Logger::messageHandler);
#endif
}


Logger::~Logger()
{
    // If we're not initialized yet, output pending messages so they don't get lost
    if (!_initialized) {
        for (auto &&message : _messages) {
            std::cerr << msgWithTime(message).constData();
        }
    }
}


std::vector<Logger::LogEntry> Logger::messages() const
{
    return _messages;
}


bool Logger::setup(bool keepMessages)
{
    _keepMessages = keepMessages;

    // Set maximum level for output (we still store/announce all messages for client-side filtering)
    if (Quassel::isOptionSet("loglevel")) {
        QString level = Quassel::optionValue("loglevel").toLower();
        if (level == "debug")
            _outputLevel = LogLevel::Debug;
        else if (level == "info")
            _outputLevel = LogLevel::Info;
        else if (level == "warning")
            _outputLevel = LogLevel::Warning;
        else if (level == "error")
            _outputLevel = LogLevel::Error;
        else {
            qCritical() << qPrintable(tr("Invalid log level %1; supported are Debug|Info|Warning|Error").arg(level));
            return false;
        }
    }

    QString logfilename = Quassel::optionValue("logfile");
    if (!logfilename.isEmpty()) {
        _logFile.setFileName(logfilename);
        if (!_logFile.open(QFile::Append|QFile::Unbuffered|QFile::Text)) {
            qCritical() << qPrintable(tr("Could not open log file \"%1\": %2").arg(logfilename, _logFile.errorString()));
        }
    }
    if (!_logFile.isOpen()) {
        if (!_logFile.open(stderr, QFile::WriteOnly|QFile::Unbuffered|QFile::Text)) {
            qCritical() << qPrintable(tr("Cannot write to stderr: %1").arg(_logFile.errorString()));
        }
    }

#ifdef HAVE_SYSLOG
    _syslogEnabled = Quassel::isOptionSet("syslog");
#endif

    _initialized = true;

    // Now that we've setup our logging backends, output pending messages
    for (auto &&message : _messages) {
        outputMessage(message);
    }
    if (!_keepMessages) {
        _messages.clear();
    }

    return true;
}


#if QT_VERSION < 0x050000
void Logger::messageHandler(QtMsgType type, const char *message)
#else
void Logger::messageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
#endif
{
    Quassel::instance()->logger()->handleMessage(type, message);
}


void Logger::handleMessage(QtMsgType type, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
        handleMessage(LogLevel::Debug, msg);
        break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
        handleMessage(LogLevel::Info, msg);
        break;
#endif
    case QtWarningMsg:
        handleMessage(LogLevel::Warning, msg);
        break;
    case QtCriticalMsg:
        handleMessage(LogLevel::Error, msg);
        break;
    case QtFatalMsg:
        handleMessage(LogLevel::Fatal, msg);
        break;
    }
}


void Logger::handleMessage(LogLevel level, const QString &msg)
{
    QString logString;

    switch (level) {
    case LogLevel::Debug:
        logString = "[Debug] ";
        break;
    case LogLevel::Info:
        logString = "[Info ] ";
        break;
    case LogLevel::Warning:
        logString = "[Warn ] ";
        break;
    case LogLevel::Error:
        logString = "[Error] ";
        break;
    case LogLevel::Fatal:
        logString = "[FATAL] ";
        break;
    }

    // Use signal connection to make this method thread-safe
    emit messageLogged({QDateTime::currentDateTime(), level, logString += msg});
}


void Logger::onMessageLogged(const LogEntry &message)
{
    if (_keepMessages) {
        _messages.push_back(message);
    }

    // If setup() wasn't called yet, just store the message - will be output later
    if (_initialized) {
        outputMessage(message);
    }
}


void Logger::outputMessage(const LogEntry &message)
{
    if (message.logLevel < _outputLevel) {
        return;
    }

#ifdef HAVE_SYSLOG
    if (_syslogEnabled) {
        int prio{LOG_INFO};
        switch (message.logLevel) {
        case LogLevel::Debug:
            prio = LOG_DEBUG;
            break;
        case LogLevel::Info:
            prio = LOG_INFO;
            break;
        case LogLevel::Warning:
            prio = LOG_WARNING;
            break;
        case LogLevel::Error:
            prio = LOG_ERR;
            break;
        case LogLevel::Fatal:
            prio = LOG_CRIT;
        }
        syslog(prio|LOG_USER, "%s", qPrintable(message.message));
    }
#endif

    if (!_logFile.fileName().isEmpty() || !_syslogEnabled) {
        _logFile.write(msgWithTime(message));
    }

#ifndef Q_OS_MAC
    // For fatal messages, write log to dump file
    if (message.logLevel == LogLevel::Fatal) {
        QFile dumpFile{Quassel::instance()->coreDumpFileName()};
        if (dumpFile.open(QIODevice::Append)) {
            dumpFile.write(msgWithTime(message));
            dumpFile.close();
        }
    }
#endif

}

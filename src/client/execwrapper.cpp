/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "execwrapper.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>

#include "client.h"
#include "messagemodel.h"
#include "quassel.h"
#include "util.h"

ExecWrapper::ExecWrapper(QObject* parent)
    : QObject(parent)
{
    connect(&_process, &QProcess::readyReadStandardOutput, this, &ExecWrapper::processReadStdout);
    connect(&_process, &QProcess::readyReadStandardError, this, &ExecWrapper::processReadStderr);
    connect(&_process, selectOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &ExecWrapper::processFinished);
    connect(&_process, &QProcess::errorOccurred, this, &ExecWrapper::processError);

    connect(this, &ExecWrapper::output, this, &ExecWrapper::postStdout);
    connect(this, &ExecWrapper::error, this, &ExecWrapper::postStderr);
}

void ExecWrapper::start(const BufferInfo& info, const QString& command)
{
    _bufferInfo = info;
    _scriptName.clear();

    QStringList params;

    static const QRegularExpression rx{R"(^\s*(\S+)(\s+(.*))?$)"};
    auto match = rx.match(command);
    if (!match.hasMatch()) {
        emit error(tr("Invalid command string for /exec: %1").arg(command));
    }
    else {
        _scriptName = match.captured(1);
        static const QRegularExpression splitRx{"\\s+"};
        params = match.captured(3).split(splitRx, Qt::SkipEmptyParts);
    }

    // Make sure we don't execute something outside a script dir
    if (_scriptName.contains("../") || _scriptName.contains("..\\")) {
        emit error(tr(R"(Name "%1" is invalid: ../ or ..\ are not allowed!)").arg(_scriptName));
    }
    else if (!_scriptName.isEmpty()) {
        foreach (QString scriptDir, Quassel::scriptDirPaths()) {
            QString fileName = scriptDir + _scriptName;
            if (!QFile::exists(fileName))
                continue;
            _process.setWorkingDirectory(scriptDir);
            _process.start(fileName, params);
            return;
        }
        emit error(tr("Could not find script \"%1\"").arg(_scriptName));
    }

    deleteLater();  // self-destruct
}

void ExecWrapper::postStdout(const QString& msg)
{
    if (_bufferInfo.isValid())
        Client::userInput(_bufferInfo, msg);
}

void ExecWrapper::postStderr(const QString& msg)
{
    if (_bufferInfo.isValid())
        Client::messageModel()->insertErrorMessage(_bufferInfo, msg);
}

void ExecWrapper::processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        emit error(tr("Script \"%1\" crashed with exit code %2.").arg(_scriptName).arg(exitCode));
    }

    // empty buffers
    if (!_stdoutBuffer.isEmpty()) {
        for (const QString& msg : _stdoutBuffer.split('\n')) {
            emit output(msg);
        }
    }
    if (!_stderrBuffer.isEmpty()) {
        for (const QString& msg : _stderrBuffer.split('\n')) {
            emit error(msg);
        }
    }

    deleteLater();
}

void ExecWrapper::processError(QProcess::ProcessError err)
{
    if (err == QProcess::FailedToStart)
        emit error(tr("Script \"%1\" could not start.").arg(_scriptName));
    else
        emit error(tr("Script \"%1\" caused error %2.").arg(_scriptName).arg(err));

    if (_process.state() != QProcess::Running)
        deleteLater();
}

void ExecWrapper::processReadStdout()
{
    QStringDecoder decoder(QStringConverter::System);
    QString str = decoder.decode(_process.readAllStandardOutput());
    str.replace(QRegularExpression("\r\n?"), "\n");
    _stdoutBuffer.append(str);
    int idx;
    while ((idx = _stdoutBuffer.indexOf('\n')) >= 0) {
        emit output(_stdoutBuffer.left(idx));
        _stdoutBuffer = _stdoutBuffer.mid(idx + 1);
    }
}

void ExecWrapper::processReadStderr()
{
    QStringDecoder decoder(QStringConverter::System);
    QString str = decoder.decode(_process.readAllStandardError());
    str.replace(QRegularExpression("\r\n?"), "\n");
    _stderrBuffer.append(str);
    int idx;
    while ((idx = _stderrBuffer.indexOf('\n')) >= 0) {
        emit error(_stderrBuffer.left(idx));
        _stderrBuffer = _stderrBuffer.mid(idx + 1);
    }
}

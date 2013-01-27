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

#include <QFile>
#include <QTextCodec>

#include "execwrapper.h"

#include "client.h"
#include "messagemodel.h"
#include "quassel.h"

ExecWrapper::ExecWrapper(QObject *parent) : QObject(parent)
{
    connect(&_process, SIGNAL(readyReadStandardOutput()), SLOT(processReadStdout()));
    connect(&_process, SIGNAL(readyReadStandardError()), SLOT(processReadStderr()));
    connect(&_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)));
    connect(&_process, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));

    connect(this, SIGNAL(output(QString)), SLOT(postStdout(QString)));
    connect(this, SIGNAL(error(QString)), SLOT(postStderr(QString)));
}


void ExecWrapper::start(const BufferInfo &info, const QString &command)
{
    _bufferInfo = info;
    QString params;

    QRegExp rx("^\\s*(\\S+)(\\s+(.*))?$");
    if (!rx.exactMatch(command)) {
        emit error(tr("Invalid command string for /exec: %1").arg(command));
    }
    else {
        _scriptName = rx.cap(1);
        params = rx.cap(3);
    }

    // Make sure we don't execute something outside a script dir
    if (_scriptName.contains("../") || _scriptName.contains("..\\"))
        emit error(tr("Name \"%1\" is invalid: ../ or ..\\ are not allowed!").arg(_scriptName));

    else {
        foreach(QString scriptDir, Quassel::scriptDirPaths()) {
            QString fileName = scriptDir + _scriptName;
            if (!QFile::exists(fileName))
                continue;
            _process.setWorkingDirectory(scriptDir);
            _process.start('"' + fileName + "\" " + params);
            return;
        }
        emit error(tr("Could not find script \"%1\"").arg(_scriptName));
    }

    deleteLater(); // self-destruct
}


void ExecWrapper::postStdout(const QString &msg)
{
    if (_bufferInfo.isValid())
        Client::userInput(_bufferInfo, msg);
}


void ExecWrapper::postStderr(const QString &msg)
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
    if (!_stdoutBuffer.isEmpty())
        foreach(QString msg, _stdoutBuffer.split('\n'))
        emit output(msg);
    if (!_stderrBuffer.isEmpty())
        foreach(QString msg, _stderrBuffer.split('\n'))
        emit error(msg);

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
    QString str = QTextCodec::codecForLocale()->toUnicode(_process.readAllStandardOutput());
    str.replace(QRegExp("\r\n?"), "\n");
    _stdoutBuffer.append(str);
    int idx;
    while ((idx = _stdoutBuffer.indexOf('\n')) >= 0) {
        emit output(_stdoutBuffer.left(idx));
        _stdoutBuffer = _stdoutBuffer.mid(idx + 1);
    }
}


void ExecWrapper::processReadStderr()
{
    QString str = QTextCodec::codecForLocale()->toUnicode(_process.readAllStandardError());
    str.replace(QRegExp("\r\n?"), "\n");
    _stderrBuffer.append(str);
    int idx;
    while ((idx = _stderrBuffer.indexOf('\n')) >= 0) {
        emit error(_stderrBuffer.left(idx));
        _stderrBuffer = _stderrBuffer.mid(idx + 1);
    }
}

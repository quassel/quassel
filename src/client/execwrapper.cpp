/***************************************************************************
*   Copyright (C) 2005-09 by the Quassel Project                          *
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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QFile>

#include "execwrapper.h"

#include "client.h"
#include "messagemodel.h"
#include "quassel.h"

ExecWrapper::ExecWrapper(QObject* parent) : QObject(parent) {
  connect(&_process, SIGNAL(readyReadStandardOutput()), SLOT(processReadStdout()));
  connect(&_process, SIGNAL(readyReadStandardError()), SLOT(processReadStderr()));
  connect(&_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)));
  connect(&_process, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));

  connect(this, SIGNAL(stdout(QString)), SLOT(postStdout(QString)));
  connect(this, SIGNAL(stderr(QString)), SLOT(postStderr(QString)));
}

void ExecWrapper::start(const BufferInfo &info, const QString &scriptName, const QStringList& params) {
  _bufferInfo = info;
  _scriptName = scriptName;
  foreach(QString scriptDir, Quassel::scriptDirPaths()) {
    QString fileName = scriptDir + '/' + scriptName;
    if(!QFile::exists(fileName))
      continue;
    _process.start(fileName, params);
    return;
  }
  emit stderr(tr("Could not find script \"%1\"").arg(scriptName));
  deleteLater();
}

void ExecWrapper::postStdout(const QString &msg) {
  if(_bufferInfo.isValid())
    Client::userInput(_bufferInfo, msg);
}

void ExecWrapper::postStderr(const QString &msg) {
  if(_bufferInfo.isValid())
    Client::messageModel()->insertErrorMessage(_bufferInfo, msg);
}

void ExecWrapper::processFinished(int exitCode, QProcess::ExitStatus status) {
  if(status == QProcess::CrashExit) {
    emit stderr(tr("Script \"%1\" crashed with exit code %2.").arg(_scriptName).arg(exitCode));
  }

  // TODO empty buffers

  deleteLater();
}

void ExecWrapper::processError(QProcess::ProcessError error) {
  emit stderr(tr("Script \"%1\" caused error %2.").arg(_scriptName).arg(error));
}

void ExecWrapper::processReadStdout() {
  _stdoutBuffer.append(_process.readAllStandardOutput());
  int idx;
  while((idx = _stdoutBuffer.indexOf('\n')) >= 0) {
    emit stdout(_stdoutBuffer.left(idx));
    _stdoutBuffer = _stdoutBuffer.mid(idx + 1);
  }
}

void ExecWrapper::processReadStderr() {
  _stderrBuffer.append(_process.readAllStandardError());
  int idx;
  while((idx = _stderrBuffer.indexOf('\n')) >= 0) {
    emit stdout(_stderrBuffer.left(idx));
    _stderrBuffer = _stderrBuffer.mid(idx + 1);
  }
}

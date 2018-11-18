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

#ifndef EXECWRAPPER_H_
#define EXECWRAPPER_H_

#include <QProcess>

#include "bufferinfo.h"

class ExecWrapper : public QObject
{
    Q_OBJECT

public:
    ExecWrapper(QObject* parent = nullptr);

public slots:
    void start(const BufferInfo& info, const QString& command);

signals:
    void error(const QString& errorMsg);
    void output(const QString& out);

private slots:
    void processReadStdout();
    void processReadStderr();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError);

    void postStdout(const QString&);
    void postStderr(const QString&);

private:
    QProcess _process;
    BufferInfo _bufferInfo;
    QString _scriptName;
    QString _stdoutBuffer;
    QString _stderrBuffer;
};

#endif

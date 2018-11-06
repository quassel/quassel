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

#include "debuglogdlg.h"

#include "quassel.h"

DebugLogDlg::DebugLogDlg(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui.textEdit->setReadOnly(true);

    connect(Quassel::instance()->logger(), SIGNAL(messageLogged(Logger::LogEntry)), SLOT(logUpdated(Logger::LogEntry)));

    QString content;
    for (auto &&message : Quassel::instance()->logger()->messages()) {
        content += toString(message);
    }
    ui.textEdit->setPlainText(content);

}


QString DebugLogDlg::toString(const Logger::LogEntry &msg)
{
    return msg.timeStamp.toString("yyyy-MM-dd hh:mm:ss ") + msg.message + "\n";
}


void DebugLogDlg::logUpdated(const Logger::LogEntry &msg)
{
    ui.textEdit->moveCursor(QTextCursor::End);
    ui.textEdit->insertPlainText(toString(msg));
    ui.textEdit->moveCursor(QTextCursor::End);
}

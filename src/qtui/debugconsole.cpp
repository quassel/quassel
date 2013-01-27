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

#include "debugconsole.h"
#include "client.h"
#include "signalproxy.h"

DebugConsole::DebugConsole(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    Client::signalProxy()->attachSignal(this, SIGNAL(scriptRequest(QString)));
    Client::signalProxy()->attachSlot(SIGNAL(scriptResult(QString)), this, SLOT(scriptResult(QString)));
}


DebugConsole::~DebugConsole()
{
}


void DebugConsole::on_evalButton_clicked()
{
    if (ui.selectCore->isChecked()) {
        emit scriptRequest(ui.scriptEdit->toPlainText());
    }
}


void DebugConsole::scriptResult(QString result)
{
    ui.resultLabel->setText(result);
}

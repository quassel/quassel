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

#ifndef DEBUGCONSOLE_H
#define DEBUGCONSOLE_H

#include "ui_debugconsole.h"

class DebugConsole : public QDialog
{
    Q_OBJECT

public:
    DebugConsole(QWidget *parent = 0);
    virtual ~DebugConsole();

public slots:
    void scriptResult(QString result);

signals:
    void scriptRequest(QString script);

private slots:
    void on_evalButton_clicked();

private:
    Ui::DebugConsole ui;
};


#endif

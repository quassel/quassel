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

#ifndef RECEIVEFILEDLG_H
#define RECEIVEFILEDLG_H

#include "ui_receivefiledlg.h"

class Transfer;

class ReceiveFileDlg : public QDialog
{
    Q_OBJECT

public:
    ReceiveFileDlg(const Transfer* transfer, QWidget* parent = nullptr);

private slots:
    void on_buttonBox_clicked(QAbstractButton* button);

private:
    Ui::ReceiveFileDlg ui;
    const Transfer* _transfer;
};

#endif

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

#include "receivefiledlg.h"

#include <QDir>
#include <QFileDialog>

#include "transfer.h"

ReceiveFileDlg::ReceiveFileDlg(const Transfer* transfer, QWidget* parent)
    : QDialog(parent)
    , _transfer(transfer)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui.setupUi(this);

    QString label
        = tr("<b>%1</b> wants to send you a file:<br>%2 (%3 bytes)").arg(transfer->nick(), transfer->fileName()).arg(transfer->fileSize());
    ui.infoText->setText(label);
}

void ReceiveFileDlg::on_buttonBox_clicked(QAbstractButton* button)
{
    if (ui.buttonBox->standardButton(button) == QDialogButtonBox::Save) {
        QString name = QFileDialog::getSaveFileName(this, QString(), QDir::currentPath() + "/" + _transfer->fileName());
        _transfer->accept(name);
    }
}

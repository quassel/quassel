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

#ifndef SIMPLENETWORKEDITOR_H
#define SIMPLENETWORKEDITOR_H

#include "ui_simplenetworkeditor.h"

#include "network.h"

class SimpleNetworkEditor : public QWidget
{
    Q_OBJECT

public:
    SimpleNetworkEditor(QWidget *parent = 0);

    void displayNetworkInfo(const NetworkInfo &networkInfo);
    void saveToNetworkInfo(NetworkInfo &networkInfo);

    QStringList defaultChannels() const;
    void setDefaultChannels(const QStringList &channels);

signals:
    void widgetHasChanged();

private slots:
    // code duplication from settingspages/networkssettingspage.{h|cpp}
    void on_serverList_itemSelectionChanged();
    void on_addServer_clicked();
    void on_deleteServer_clicked();
    void on_editServer_clicked();
    void on_upServer_clicked();
    void on_downServer_clicked();

    void setWidgetStates();

private:
    Ui::SimpleNetworkEditor ui;

    NetworkInfo _networkInfo;
};


#endif //SIMPLENETWORKEDITOR_H

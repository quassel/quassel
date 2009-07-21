/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef CONNECTIONSETTINGSPAGE_H_
#define CONNECTIONSETTINGSPAGE_H_

#include "settings.h"
#include "settingspage.h"

#include "ui_connectionsettingspage.h"

class ConnectionSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    ConnectionSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

  public slots:

  private slots:
    void clientConnected();
    void clientDisconnected();
    void initDone();

  private:
    QVariant loadAutoWidgetValue(const QString &widgetName);
    void saveAutoWidgetValue(const QString &widgetName, const QVariant &value);

    Ui::ConnectionSettingsPage ui;
};

#endif

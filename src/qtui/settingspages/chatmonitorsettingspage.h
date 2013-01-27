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

#ifndef _CHATMONITORSETTINGSPAGE_H_
#define _CHATMONITORSETTINGSPAGE_H_

#include "settingspage.h"
#include "ui_chatmonitorsettingspage.h"

#include <QHash>

class BufferViewConfig;

class ChatMonitorSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    ChatMonitorSettingsPage(QWidget *parent = 0);
    bool hasDefaults() const;

public slots:
    void save();
    void load();
    void loadSettings();
    void defaults();

private slots:
    void widgetHasChanged();
    void on_activateBuffer_clicked();
    void on_deactivateBuffer_clicked();
    void switchOperationMode(int idx);

private:
    Ui::ChatMonitorSettingsPage ui;
    QHash<QString, QVariant> settings;
    bool testHasChanged();
    void toggleBuffers(BufferView *inView, BufferViewConfig *inCfg, BufferView *outView, BufferViewConfig *outCfg);

    BufferViewConfig *_configAvailable;
    BufferViewConfig *_configActive;
};


#endif

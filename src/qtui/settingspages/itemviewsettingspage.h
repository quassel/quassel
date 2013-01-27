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

#ifndef ITEMVIEWSETTINGSPAGE_H_
#define ITEMVIEWSETTINGSPAGE_H_

#include "settingspage.h"
#include "ui_itemviewsettingspage.h"

class ColorButton;
class QSignalMapper;
class QTreeWidgetItem;

class ItemViewSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    ItemViewSettingsPage(QWidget *parent = 0);

    inline bool hasDefaults() const { return true; }

public slots:
    void save();

private slots:
    void updateBufferViewPreview(QWidget *button);

private:
    Ui::ItemViewSettingsPage ui;
    QSignalMapper *_mapper;
    QTreeWidgetItem *_networkItem, *_defaultBufferItem, *_inactiveBufferItem,
    *_activeBufferItem, *_unreadBufferItem, *_highlightedBufferItem;

    inline QString settingsKey() const { return QString("ItemViews"); }
};


#endif

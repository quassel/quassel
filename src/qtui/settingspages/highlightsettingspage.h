/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef _HIGHLIGHTSETTINGSPAGE_H_
#define _HIGHLIGHTSETTINGSPAGE_H_

#include <QVariantList>
#include <QTableWidgetItem>

#include "settingspage.h"
#include "ui_highlightsettingspage.h"

class HighlightSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    HighlightSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

public slots:
    void save();
    void load();
    void defaults();

private slots:
    void widgetHasChanged();
    void addNewRow(QString name = tr("highlight rule"), bool regex = false, bool cs = false, bool enable = true, QString chanName = "", bool self = false);
    void removeSelectedRows();
    void selectRow(QTableWidgetItem *item);
    void tableChanged(QTableWidgetItem *item);

private:
    Ui::HighlightSettingsPage ui;
    QVariantList highlightList;
    // QVariant -> QHash<QString, QVariant>:
    //    regex:  bool
    //    name:   QString
    //    enable: bool
    enum column {
        NameColumn = 0,
        RegExColumn = 1,
        CsColumn = 2,
        EnableColumn = 3,
        ChanColumn = 4,
        ColumnCount = 5
    };

    void emptyTable();

    bool testHasChanged();
};


#endif

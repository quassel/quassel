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
    HighlightSettingsPage(QWidget *parent = nullptr);

    bool hasDefaults() const override;

public slots:
    void save() override;
    void load() override;
    void defaults() override;

private slots:
    void widgetHasChanged();
    void addNewRow(QString name = tr("highlight rule"), bool regex = false, bool cs = false, bool enable = true, QString chanName = "", bool self = false);
    void removeSelectedRows();
    void selectRow(QTableWidgetItem *item);
    void tableChanged(QTableWidgetItem *item);

    /**
     * Event handler for Local Highlights Details button
     */
    void on_localHighlightsDetails_clicked();

private:
    Ui::HighlightSettingsPage ui;
    QVariantList highlightList;
    // QVariant -> QHash<QString, QVariant>:
    //    regex:  bool
    //    name:   QString
    //    enable: bool
    enum column {
        EnableColumn = 0,
        NameColumn = 1,
        RegExColumn = 2,
        CsColumn = 3,
        ChanColumn = 4,
        ColumnCount = 5
    };

    void emptyTable();

    bool testHasChanged();
};


#endif

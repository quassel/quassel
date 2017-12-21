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

#pragma once

#include <QVariantList>
#include <QTableWidgetItem>

#include "highlightrulemanager.h"
#include "settingspage.h"

#include "ui_corehighlightsettingspage.h"

class CoreHighlightSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    explicit CoreHighlightSettingsPage(QWidget *parent = nullptr);

    bool hasDefaults() const override;

    bool isSelectable() const override;

public slots:
    void save() override;
    void load() override;
    void defaults() override;
    void revert();
    void clientConnected();

private slots:
    void coreConnectionStateChanged(bool state);
    void widgetHasChanged();
    void addNewHighlightRow(bool enable = true, const QString &name = tr("highlight rule"), bool regex = false,
                            bool cs = false, const QString &sender = "", const QString &chanName = "",
                            bool self = false);
    void addNewIgnoredRow(bool enable = true, const QString &name = tr("highlight rule"), bool regex = false,
                          bool cs = false, const QString &sender = "", const QString &chanName = "", bool self = false);
    void removeSelectedHighlightRows();
    void removeSelectedIgnoredRows();
    void selectHighlightRow(QTableWidgetItem *item);
    void selectIgnoredRow(QTableWidgetItem *item);
    void highlightTableChanged(QTableWidgetItem *item);
    void ignoredTableChanged(QTableWidgetItem *item);

private:
    Ui::CoreHighlightSettingsPage ui;

    HighlightRuleManager::HighlightRuleList highlightList;
    HighlightRuleManager::HighlightRuleList ignoredList;

    enum column {
        EnableColumn = 0,
        NameColumn = 1,
        RegExColumn = 2,
        CsColumn = 3,
        SenderColumn = 4,
        ChanColumn = 5,
        ColumnCount = 6
    };

    void emptyHighlightTable();
    void emptyIgnoredTable();

    void setupRuleTable(QTableWidget *highlightTable) const;

    void importRules();

    bool _initialized;
};

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

#pragma once

#include <QTableWidgetItem>
#include <QVariantList>

#include "highlightrulemanager.h"
#include "settingspage.h"

#include "ui_corehighlightsettingspage.h"

class CoreHighlightSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    explicit CoreHighlightSettingsPage(QWidget* parent = nullptr);

    bool hasDefaults() const override;

    bool isSelectable() const override;

public slots:
    void save() final override;
    void load() final override;
    void defaults() final override;
    void revert();
    void clientConnected();

private slots:
    void coreConnectionStateChanged(bool state);
    void widgetHasChanged();
    void addNewHighlightRow(bool enable = true,
                            int id = -1,
                            const QString& name = tr("highlight rule"),
                            bool regex = false,
                            bool cs = false,
                            const QString& sender = "",
                            const QString& chanName = "",
                            bool self = false);
    void addNewIgnoredRow(bool enable = true,
                          int id = -1,
                          const QString& name = tr("highlight rule"),
                          bool regex = false,
                          bool cs = false,
                          const QString& sender = "",
                          const QString& chanName = "",
                          bool self = false);
    void removeSelectedHighlightRows();
    void removeSelectedIgnoredRows();
    void highlightNicksChanged(int index);
    void selectHighlightRow(QTableWidgetItem* item);
    void selectIgnoredRow(QTableWidgetItem* item);
    void highlightTableChanged(QTableWidgetItem* item);
    void ignoredTableChanged(QTableWidgetItem* item);

    /** Import local Highlight rules into the Core Highlight rules
     *
     * Iterates through all local highlight rules, converting each into core-side highlight rules.
     */
    void importRules();

    /**
     * Event handler for core unspported Details button
     */
    void on_coreUnsupportedDetails_clicked();

private:
    Ui::CoreHighlightSettingsPage ui;

    HighlightRuleManager::HighlightRuleList highlightList;
    HighlightRuleManager::HighlightRuleList ignoredList;

    enum column
    {
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

    void setupRuleTable(QTableWidget* highlightTable) const;

    /**
     * Get tooltip for the specified rule table column
     *
     * @param tableColumn Column to retrieve tooltip
     * @return Translated tooltip for the specified column
     */
    QString getTableTooltip(column tableColumn) const;

    /**
     * Setup tooltips and "What's this?" prompts for table entries
     *
     * @param enableWidget  Enabled checkbox
     * @param nameWidget    Rule name
     * @param regExWidget   RegEx enabled
     * @param csWidget      Case-sensitive
     * @param senderWidget  Sender name
     * @param chanWidget    Channel name
     */
    void setupTableTooltips(
        QWidget* enableWidget, QWidget* nameWidget, QWidget* regExWidget, QWidget* csWidget, QWidget* senderWidget, QWidget* chanWidget) const;

    /**
     * Setup tooltips and "What's this?" prompts for table entries
     *
     * @param enableWidget  Enabled checkbox
     * @param nameWidget    Rule name
     * @param regExWidget   RegEx enabled
     * @param csWidget      Case-sensitive
     * @param senderWidget  Sender name
     * @param chanWidget    Channel name
     */
    void setupTableTooltips(QTableWidgetItem* enableWidget,
                            QTableWidgetItem* nameWidget,
                            QTableWidgetItem* regExWidget,
                            QTableWidgetItem* csWidget,
                            QTableWidgetItem* senderWidget,
                            QTableWidgetItem* chanWidget) const;

    /** Update the UI to show core support for highlights
     *
     * Shows or hides the UI warnings around core-side highlights according to core connection and
     * core feature support.
     *
     * @param state  True if connected to core, otherwise false
     */
    void updateCoreSupportStatus(bool state);

    int nextId();

    bool _initialized;
};

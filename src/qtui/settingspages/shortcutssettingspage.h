/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#ifndef SHORTCUTSSETTINGSPAGE_H
#define SHORTCUTSSETTINGSPAGE_H

#include <QSortFilterProxyModel>

#include "settingspage.h"

#include "ui_shortcutssettingspage.h"

class ActionCollection;
class ShortcutsModel;

class ShortcutsFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ShortcutsFilter(QObject* parent = nullptr);

public slots:
    void setFilterString(const QString& filterString);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    QString _filterString;
};

class ShortcutsSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    ShortcutsSettingsPage(const QHash<QString, ActionCollection*>& actionCollections, QWidget* parent = nullptr);

    inline bool hasDefaults() const override { return true; }

public slots:
    void save() override;
    void load() override;
    void defaults() override;

private slots:
    void on_searchEdit_textChanged(const QString& text);
    void keySequenceChanged(const QKeySequence& seq, const QModelIndex& conflicting);
    void setWidgetStates();
    void toggledCustomOrDefault();

private:
    Ui::ShortcutsSettingsPage ui;
    ShortcutsModel* _shortcutsModel;
    ShortcutsFilter* _shortcutsFilter;
};

#endif  // SHORTCUTSSETTINGSPAGE_H

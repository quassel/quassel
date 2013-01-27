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
    ShortcutsFilter(QObject *parent = 0);

public slots:
    void setFilterString(const QString &filterString);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QString _filterString;
};


class ShortcutsSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    ShortcutsSettingsPage(const QHash<QString, ActionCollection *> &actionCollections, QWidget *parent = 0);

    inline bool hasDefaults() const { return true; }

public slots:
    void save();
    void load();
    void defaults();

private slots:
    void on_searchEdit_textChanged(const QString &text);
    void keySequenceChanged(const QKeySequence &seq, const QModelIndex &conflicting);
    void setWidgetStates();
    void toggledCustomOrDefault();

private:
    Ui::ShortcutsSettingsPage ui;
    ShortcutsModel *_shortcutsModel;
    ShortcutsFilter *_shortcutsFilter;
};


#endif // SHORTCUTSSETTINGSPAGE_H

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

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>

#include "ui_settingsdlg.h"

#include "settingspage.h"

class SettingsDlg : public QDialog
{
    Q_OBJECT

public:
    SettingsDlg(QWidget *parent = 0);
    void registerSettingsPage(SettingsPage *);
    void unregisterSettingsPage(SettingsPage *);

    inline SettingsPage *currentPage() const { return _currentPage; }

public slots:
    void selectPage(SettingsPage *sp); // const QString &category, const QString &title);

private slots:
    void coreConnectionStateChanged();
    void itemSelected();
    void buttonClicked(QAbstractButton *);
    bool applyChanges();
    void undoChanges();
    void reload();
    void loadDefaults();
    void setButtonStates();
    void setItemState(QTreeWidgetItem *);

private:
    Ui::SettingsDlg ui;

    SettingsPage *_currentPage;
    QHash<SettingsPage *, bool> pageIsLoaded;

    enum {
        SettingsPageRole = Qt::UserRole
    };
};


#endif

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

#ifndef APPEARANCESETTINGSPAGE_H
#define APPEARANCESETTINGSPAGE_H

#include <QHash>
#include <QList>
#include <QLocale>
#include <QTextCharFormat>

#include "settings.h"
#include "settingspage.h"
#include "ui_appearancesettingspage.h"

class AppearanceSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    AppearanceSettingsPage(QWidget *parent = 0);

    inline bool hasDefaults() const { return true; }

public slots:
    void save();
    void load();
    void defaults();

private slots:
    void widgetHasChanged();

    void chooseStyleSheet();

private:
    bool testHasChanged();
    void initStyleComboBox();
    void initLanguageComboBox();
    void initIconThemeComboBox();
    QLocale selectedLocale() const;
    QString selectedIconTheme() const;

    Ui::AppearanceSettingsPage ui;
    QHash<QString, QVariant> settings;
    QMap<QString, QLocale> _locales;

    inline QString settingsKey() const { return QString("QtUi"); }
};


#endif

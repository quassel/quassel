/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _COLORSETTINGSPAGE_H_
#define _COLORSETTINGSPAGE_H_

#include <QHash>

#include "settingspage.h"
#include "ui_colorsettingspage.h"
#include "uistyle.h"

class QSignalMapper;
class ColorButton;

class ColorSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    ColorSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

  public slots:
    void save();
    void load();
    void defaults();
    void defaultBufferview();
    void defaultServerActivity();
    void defaultUserActivity();
    void defaultMessage();
    void defaultMircColorCodes();
    void defaultNickview();

  private slots:
    void widgetHasChanged();
    void chooseColor(QWidget *button);

  private:
    Ui::ColorSettingsPage ui;
    QHash<QString, QVariant> settings;
    QSignalMapper *mapper;

    bool testHasChanged();
    void chatviewPreview();
    void bufferviewPreview();
    void saveColor(UiStyle::FormatType formatType, const QColor &foreground, const QColor &background, bool enableBG = true);
    void saveMircColor(int num, const QColor &color);
};

#endif

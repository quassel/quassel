/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _FONTSSETTINGSPAGE_H_
#define _FONTSSETTINGSPAGE_H_

#include <QHash>
#include <QTextCharFormat>

#include "settings.h"
#include "settingspage.h"

#include "ui_fontssettingspage.h"

class QSignalMapper;
class QLabel;

class FontsSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    FontsSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

  public slots:
    void save();
    void load();
    void defaults();

  private slots:
    void load(Settings::Mode mode);
    void initLabel(QLabel *label, const QFont &font);
    void setFont(QLabel *label, const QFont &font);
    void chooseFont(QWidget *label);

    void widgetHasChanged();

  private:
    void clearFontFromFormat(QTextCharFormat &fmt);

    Ui::FontsSettingsPage ui;

    QSignalMapper *mapper;

};

#endif

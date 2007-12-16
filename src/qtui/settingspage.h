/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _SETTINGSPAGE_H_
#define _SETTINGSPAGE_H_

#include <QWidget>

//! A SettingsPage is a page in the settings dialog.
class SettingsPage : public QWidget {
  Q_OBJECT

  public:
    SettingsPage(const QString &category, const QString &name, QWidget *parent = 0);
    virtual ~SettingsPage() {};
    virtual QString category() const;
    virtual QString name() const;

    virtual bool hasChanged() const = 0;

  public slots:
    virtual void save() = 0;
    virtual void load() = 0;
    virtual void defaults() = 0;

  signals:
    void changed(bool hasChanged = true);

};

#endif

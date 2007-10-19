/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#ifndef _SETTINGSPAGE_H_
#define _SETTINGSPAGE_H_

//! Interface that needs to be implemented by pages of the settings dialog.
class SettingsPage {
  public:
    virtual ~SettingsPage() {};
    virtual QString category() = 0;
    virtual QString title() = 0;
    virtual QWidget *widget() = 0;
    virtual void saveSettings() = 0;
    virtual void loadSettings() = 0;

    virtual bool hasChanged() = 0;

  signals:
    void modified();

};

Q_DECLARE_INTERFACE(SettingsPage, "org.quassel-irc.iface.SettingsPage/1.0");

#endif

/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef QTUISETTINGS_H_
#define QTUISETTINGS_H_

#include <QColor>

#include "uisettings.h"

class QtUiSettings : public UiSettings {
public:
  QtUiSettings(const QString &subGroup);
  QtUiSettings();
};

class QtUiStyleSettings : public UiSettings {
public:
  QtUiStyleSettings(const QString &subGroup);
  QtUiStyleSettings();
};

class WarningsSettings : public UiSettings {
public:
  WarningsSettings();

  bool showWarning(const QString &key);
  void setShowWarning(const QString &key, bool show);
};

#endif

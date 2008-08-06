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

#ifndef _QTUISETTINGS_H_
#define _QTUISETTINGS_H_

#include <QColor>

#include "uisettings.h"

class QtUiSettings : public UiSettings {

  public: 
    QtUiSettings(const QString &group = "QtUi");

}; 

class QtUiStyleSettings : public UiStyleSettings {

  public:
    QtUiStyleSettings(const QString &group = "QtUiStyle");

    void setHighlightColor(const QColor &);
    QColor highlightColor();

};

#endif

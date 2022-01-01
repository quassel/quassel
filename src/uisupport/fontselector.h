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

#pragma once

#include "uisupport-export.h"

#include <QLabel>
#include <QWidget>

class UISUPPORT_EXPORT FontSelector : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QFont selectedFont READ selectedFont WRITE setSelectedFont)

public:
    FontSelector(QWidget* parent = nullptr);

    inline const QFont& selectedFont() const { return _font; }

public slots:
    void setSelectedFont(const QFont& font);

signals:
    void fontChanged(const QFont&);

protected:
    void changeEvent(QEvent* e) override;

protected slots:
    void chooseFont();

private:
    QFont _font;
    QLabel* _demo;
};

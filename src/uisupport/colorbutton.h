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

#ifndef COLORBUTTON_H_
#define COLORBUTTON_H_

#include <QToolButton>

class ColorButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor USER true)

public :
        explicit ColorButton(QWidget *parent = 0);
    explicit ColorButton(const QColor &c, QWidget *parent = 0);

    void setColor(const QColor &color);
    QColor color() const;

signals:
    void colorChanged(const QColor &);

protected:
    //void paintEvent(QPaintEvent *event);

private slots:
    void chooseColor();

private:
    QColor _color;
};


#endif

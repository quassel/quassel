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

#ifndef VERTICALDOCKTITLE_H
#define VERTICALDOCKTITLE_H

#include <QDockWidget>
#include <QSize>

class VerticalDockTitle : public QWidget
{
    Q_OBJECT

public:
    VerticalDockTitle(QDockWidget *parent);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    void show(bool show_);

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    bool _show;
};


class EmptyDockTitle : public QWidget
{
    Q_OBJECT

public:
    inline EmptyDockTitle(QDockWidget *parent) : QWidget(parent) {}

    inline virtual QSize sizeHint() const { return QSize(0, 0); }
};


class VerticalDock : public QDockWidget
{
    Q_OBJECT

public:
    VerticalDock(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    VerticalDock(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    void showTitle(bool show);
    void setDefaultTitleWidget();
};


#endif // VERTICALDOCKTITLE_H

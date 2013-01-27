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

#include <QImage>
#include <QPainter>

#include "mainpage.h"

MainPage::MainPage(QWidget *parent) : QWidget(parent)
{
}


void MainPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QImage img(":/pics/quassel-logo.png"); // FIXME load externally

    if (img.height() > height() || img.width() > width())
        img = img.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int xmargin = (width() - img.width()) / 2;
    int ymargin = (height() - img.height()) / 2;

    painter.drawImage(xmargin, ymargin, img);
}

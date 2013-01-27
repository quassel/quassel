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

#include "clearablelineedit.h"

#include <QToolButton>
#include <QStyle>

#include "iconloader.h"

ClearableLineEdit::ClearableLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    clearButton = new QToolButton(this);
    clearButton->setIcon(SmallIcon("edit-clear-locationbar-rtl"));
#ifndef Q_WS_QWS
    clearButton->setCursor(Qt::ArrowCursor);
#endif
    clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    clearButton->hide();

    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(updateClearButton(const QString &)));

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
        qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}


void ClearableLineEdit::resizeEvent(QResizeEvent *)
{
    QSize size = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    clearButton->move(rect().right() - frameWidth - size.width(),
        (rect().bottom() + 1 - size.height())/2);
}


void ClearableLineEdit::updateClearButton(const QString &text)
{
    clearButton->setVisible(!text.isEmpty());
}

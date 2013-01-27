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

#include <QEvent>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "fontselector.h"

FontSelector::FontSelector(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    QPushButton *chooseButton = new QPushButton(tr("Choose..."), this);
    connect(chooseButton, SIGNAL(clicked()), SLOT(chooseFont()));

    layout->addWidget(_demo = new QLabel("Font"));
    layout->addWidget(chooseButton);
    layout->setContentsMargins(0, 0, 0, 0);

    _demo->setFrameStyle(QFrame::StyledPanel);
    _demo->setFrameShadow(QFrame::Sunken);
    _demo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    _font = font();
}


void FontSelector::setSelectedFont(const QFont &font)
{
    _font = font;
    _demo->setText(QString("%1 %2pt").arg(font.family()).arg(font.pointSize()));
    _demo->setFont(font);
    emit fontChanged(font);
}


void FontSelector::chooseFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, _demo->font());
    if (ok) {
        setSelectedFont(font);
    }
}


void FontSelector::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        _demo->setFont(_font);
    }
}

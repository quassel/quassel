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

#ifndef BUFFERSHORTCUTPOPUP_H
#define BUFFERSHORTCUTPOPUP_H

#include <QWidget>

#include "buffermodel.h"
#include "keysequencewidget.h"

class BufferShortcutPopup : public QWidget {
    Q_OBJECT

public:
    BufferShortcutPopup(BufferInfo, QWidget *parent = 0);

private slots:
    void onSequenceWidgetChanged(QKeySequence, QModelIndex conflicting);

signals:
    void keySequenceChanged(BufferId);

private:
    KeySequenceWidget *_keySeq;
    BufferInfo _bufferInfo;
    ShortcutsModel *_shortcutsModel;
};

#endif // BUFFERSHORTCUTPOPUP_H

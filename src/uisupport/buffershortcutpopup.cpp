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

#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QHBoxLayout>

#include "buffershortcutpopup.h"

#include "action.h"
#include "actioncollection.h"
#include "buffersettings.h"
#include "keysequencewidget.h"
#include "graphicalui.h"

BufferShortcutPopup::BufferShortcutPopup(BufferInfo bufferInfo, QWidget *parent)
    : QWidget(parent, Qt::Popup), _bufferInfo(bufferInfo)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    QLabel *label = new QLabel(tr("Enter new Quick Accessor:"));

    QHash<QString, ActionCollection *> actionCollections;
    actionCollections.unite(GraphicalUi::quickAccessorActionCollections());
    actionCollections.unite(GraphicalUi::actionCollections());
    _shortcutsModel = new ShortcutsModel(actionCollections);

    _keySeq = new KeySequenceWidget(this);
    _keySeq->setModel(_shortcutsModel);
    _keySeq->setKeySequence(BufferSettings(bufferInfo.bufferId()).shortcut());

    layout->addWidget(label);
    layout->addWidget(_keySeq);
    setLayout(layout);

    connect(_keySeq, SIGNAL(keySequenceChanged(QKeySequence, QModelIndex)), this, SLOT(onSequenceWidgetChanged(QKeySequence, QModelIndex)));

    //Move the popup to the mouse coordinates (ensure it's within the screen)
    QRect geometry = QApplication::desktop()->availableGeometry(this);
    geometry.adjust(0, 0, -sizeHint().width(), -sizeHint().height());
    QPoint pos = QCursor::pos();
    pos.rx() -= sizeHint().width()/2;
    pos.ry() -= sizeHint().height()/2;

    pos.setX(geometry.right() < pos.x() ? geometry.right() : pos.x());
    pos.setY(geometry.bottom() < pos.y() ? geometry.bottom() : pos.y());
    pos.setX(geometry.left() > pos.x() ? geometry.left() : pos.x());
    pos.setY(geometry.top() > pos.y() ? geometry.top() : pos.y());

    move(pos);
    show();
    _keySeq->startRecording();
}


void BufferShortcutPopup::onSequenceWidgetChanged(QKeySequence sequence, QModelIndex conflicting) {
    if (conflicting.isValid()) {
        //Clear the conflict
        _shortcutsModel->setData(conflicting, QKeySequence(), ShortcutsModel::ActiveShortcutRole);
        _shortcutsModel->commit();

        //We must also clear the shortcut property in BufferSettings
        QObject *actObj = qvariant_cast<QObject *>(_shortcutsModel->data(conflicting, ShortcutsModel::ActionRole));
        Action *action = qobject_cast<Action *>(actObj);
        BufferId id = qvariant_cast<BufferId>(action->property("BufferId"));
        BufferSettings s(id);
        qDebug() << s.shortcut().toString();
        s.setShortcut(QKeySequence());
    }

    BufferSettings(_bufferInfo.bufferId()).setShortcut(sequence);
    emit keySequenceChanged(_bufferInfo.bufferId());

    hide();
    deleteLater();
}


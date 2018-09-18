/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
 ***************************************************************************
 * Parts of this implementation are taken from KDE's kaction.cpp           *
 ***************************************************************************/

#include "action.h"

#include <QApplication>

Action::Action(QObject *parent)
    : QWidgetAction(parent)
{
    setProperty("isShortcutConfigurable", true);
    connect(this, &QAction::triggered, this, &Action::slotTriggered);
}


Action::Action(const QString &text, QObject *parent, const QKeySequence &shortcut)
    : Action(parent)
{
    setText(text);
    setShortcut(shortcut);
}


Action::Action(const QIcon &icon, const QString &text, QObject *parent, const QKeySequence &shortcut)
    : Action(text, parent, shortcut)
{
    setIcon(icon);
}


void Action::slotTriggered()
{
    emit triggered(QApplication::mouseButtons(), QApplication::keyboardModifiers());
}


bool Action::isShortcutConfigurable() const
{
    return property("isShortcutConfigurable").toBool();
}


void Action::setShortcutConfigurable(bool b)
{
    setProperty("isShortcutConfigurable", b);
}


QKeySequence Action::shortcut(ShortcutTypes type) const
{
    Q_ASSERT(type);
    if (type == DefaultShortcut) {
        auto sequence = property("defaultShortcuts").value<QList<QKeySequence>>();
        return sequence.isEmpty() ? QKeySequence() : sequence.first();
    }

    return shortcuts().isEmpty() ? QKeySequence() : shortcuts().first();
}


void Action::setShortcut(const QShortcut &shortcut, ShortcutTypes type)
{
    setShortcut(shortcut.key(), type);
}


void Action::setShortcut(const QKeySequence &key, ShortcutTypes type)
{
    Q_ASSERT(type);

    if (type & DefaultShortcut) {
        setProperty("defaultShortcuts", QVariant::fromValue(QList<QKeySequence>() << key));
    }
    if (type & ActiveShortcut)
        QAction::setShortcut(key);
}

/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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
 * Parts of this API have been shamelessly stolen from KDE's kaction.h     *
 ***************************************************************************/

#pragma once

#include "uisupport-export.h"

#include <type_traits>

#include <QShortcut>
#include <QWidgetAction>

#include "util.h"

/// A specialized QWidgetAction, enhanced by some KDE features
/** This declares/implements a subset of KAction's API (notably we've left out global shortcuts
 *  for now), which should make it easy to plug in KDE support later on.
 */
class UISUPPORT_EXPORT Action : public QWidgetAction
{
    Q_OBJECT

    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)
    Q_PROPERTY(bool shortcutConfigurable READ isShortcutConfigurable WRITE setShortcutConfigurable)
    Q_FLAGS(ShortcutType)

public:
    enum ShortcutType
    {
        ActiveShortcut = 0x01,
        DefaultShortcut = 0x02
    };
    Q_DECLARE_FLAGS(ShortcutTypes, ShortcutType)

    explicit Action(QObject* parent);
    Action(const QString& text, QObject* parent, const QKeySequence& shortcut = 0);
    Action(const QIcon& icon, const QString& text, QObject* parent, const QKeySequence& shortcut = 0);

    template<typename Receiver, typename Slot>
    Action(const QString& text, QObject* parent, const Receiver* receiver, Slot slot, const QKeySequence& shortcut = 0)
        : Action(text, parent, shortcut)
    {
        static_assert(!std::is_same<Slot, const char*>::value, "Old-style connects not supported");

        setShortcut(shortcut);
        connect(this, &QAction::triggered, receiver, slot);
    }

    template<typename Receiver, typename Slot>
    Action(const QIcon& icon, const QString& text, QObject* parent, const Receiver* receiver, Slot slot, const QKeySequence& shortcut = 0)
        : Action(text, parent, receiver, slot, shortcut)
    {
        setIcon(icon);
    }

    QKeySequence shortcut(ShortcutTypes types = ActiveShortcut) const;
    void setShortcut(const QShortcut& shortcut, ShortcutTypes type = ShortcutTypes(ActiveShortcut | DefaultShortcut));
    void setShortcut(const QKeySequence& shortcut, ShortcutTypes type = ShortcutTypes(ActiveShortcut | DefaultShortcut));

    bool isShortcutConfigurable() const;
    void setShortcutConfigurable(bool configurable);

signals:
    void triggered(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);

private slots:
    void slotTriggered();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Action::ShortcutTypes)

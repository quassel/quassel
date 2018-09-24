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
 * Parts of this implementation are based on KDE's KActionCollection.      *
 ***************************************************************************/

#include "actioncollection.h"

#include <QAction>
#include <QDebug>
#include <QMetaMethod>

#include "action.h"
#include "uisettings.h"

void ActionCollection::addActions(const std::vector<std::pair<QString, Action*>>& actions)
{
    for (auto&& p : actions) {
        addAction(p.first, p.second);
    }
}

#ifndef HAVE_KDE

int ActionCollection::count() const
{
    return actions().count();
}

bool ActionCollection::isEmpty() const
{
    return actions().count();
}

void ActionCollection::clear()
{
    _actionByName.clear();
    qDeleteAll(_actions);
    _actions.clear();
}

QAction* ActionCollection::action(const QString& name) const
{
    return _actionByName.value(name, 0);
}

QList<QAction*> ActionCollection::actions() const
{
    return _actions;
}

QAction* ActionCollection::addAction(const QString& name, QAction* action)
{
    if (!action)
        return action;

    const QString origName = action->objectName();
    QString indexName = name;

    if (indexName.isEmpty())
        indexName = action->objectName();
    else
        action->setObjectName(indexName);
    if (indexName.isEmpty())
        indexName = indexName.sprintf("unnamed-%p", (void*)action);

    // do we already have this action?
    if (_actionByName.value(indexName, 0) == action)
        return action;
    // or maybe another action under this name?
    if (QAction* oldAction = _actionByName.value(indexName))
        takeAction(oldAction);

    // do we already have this action under a different name?
    int oldIndex = _actions.indexOf(action);
    if (oldIndex != -1) {
        _actionByName.remove(origName);
        _actions.removeAt(oldIndex);
    }

    // add action
    _actionByName.insert(indexName, action);
    _actions.append(action);

    foreach (QWidget* widget, _associatedWidgets) {
        widget->addAction(action);
    }

    connect(action, &QObject::destroyed, this, &ActionCollection::actionDestroyed);
    if (_connectHovered)
        connect(action, &QAction::hovered, this, &ActionCollection::slotActionHovered);
    if (_connectTriggered)
        connect(action, &QAction::triggered, this, &ActionCollection::slotActionTriggered);

    emit inserted(action);
    return action;
}

void ActionCollection::removeAction(QAction* action)
{
    delete takeAction(action);
}

QAction* ActionCollection::takeAction(QAction* action)
{
    if (!unlistAction(action))
        return nullptr;

    foreach (QWidget* widget, _associatedWidgets) {
        widget->removeAction(action);
    }

    action->disconnect(this);
    return action;
}

void ActionCollection::readSettings()
{
    ShortcutSettings s;
    QStringList savedShortcuts = s.savedShortcuts();

    foreach (const QString& name, _actionByName.keys()) {
        if (!savedShortcuts.contains(name))
            continue;
        auto* action = qobject_cast<Action*>(_actionByName.value(name));
        if (action)
            action->setShortcut(s.loadShortcut(name), Action::ActiveShortcut);
    }
}

void ActionCollection::writeSettings() const
{
    ShortcutSettings s;
    foreach (const QString& name, _actionByName.keys()) {
        auto* action = qobject_cast<Action*>(_actionByName.value(name));
        if (!action)
            continue;
        if (!action->isShortcutConfigurable())
            continue;
        if (action->shortcut(Action::ActiveShortcut) == action->shortcut(Action::DefaultShortcut))
            continue;
        s.saveShortcut(name, action->shortcut(Action::ActiveShortcut));
    }
}

void ActionCollection::slotActionTriggered()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (action)
        emit actionTriggered(action);
}

void ActionCollection::slotActionHovered()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (action)
        emit actionHovered(action);
}

void ActionCollection::actionDestroyed(QObject* obj)
{
    // remember that this is not an QAction anymore at this point
    auto* action = static_cast<QAction*>(obj);

    unlistAction(action);
}

void ActionCollection::connectNotify(const QMetaMethod& signal)
{
    if (_connectHovered && _connectTriggered)
        return;

    if (QMetaMethod::fromSignal(&ActionCollection::actionHovered) == signal) {
        if (!_connectHovered) {
            _connectHovered = true;
            foreach (QAction* action, actions())
                connect(action, &QAction::hovered, this, &ActionCollection::slotActionHovered);
        }
    }
    else if (QMetaMethod::fromSignal(&ActionCollection::actionTriggered) == signal) {
        if (!_connectTriggered) {
            _connectTriggered = true;
            foreach (QAction* action, actions())
                connect(action, &QAction::triggered, this, &ActionCollection::slotActionTriggered);
        }
    }

    QObject::connectNotify(signal);
}

void ActionCollection::associateWidget(QWidget* widget) const
{
    foreach (QAction* action, actions()) {
        if (!widget->actions().contains(action))
            widget->addAction(action);
    }
}

void ActionCollection::addAssociatedWidget(QWidget* widget)
{
    if (!_associatedWidgets.contains(widget)) {
        widget->addActions(actions());
        _associatedWidgets.append(widget);
        connect(widget, &QObject::destroyed, this, &ActionCollection::associatedWidgetDestroyed);
    }
}

void ActionCollection::removeAssociatedWidget(QWidget* widget)
{
    foreach (QAction* action, actions())
        widget->removeAction(action);
    _associatedWidgets.removeAll(widget);
    disconnect(widget, &QObject::destroyed, this, &ActionCollection::associatedWidgetDestroyed);
}

QList<QWidget*> ActionCollection::associatedWidgets() const
{
    return _associatedWidgets;
}

void ActionCollection::clearAssociatedWidgets()
{
    foreach (QWidget* widget, _associatedWidgets)
        foreach (QAction* action, actions())
            widget->removeAction(action);

    _associatedWidgets.clear();
}

void ActionCollection::associatedWidgetDestroyed(QObject* obj)
{
    _associatedWidgets.removeAll(static_cast<QWidget*>(obj));
}

bool ActionCollection::unlistAction(QAction* action)
{
    // This might be called with a partly destroyed QAction!

    int index = _actions.indexOf(action);
    if (index == -1)
        return false;

    QString name = action->objectName();
    _actionByName.remove(name);
    _actions.removeAt(index);

    // TODO: remove from ActionCategory if we ever get that

    return true;
}

#endif /* HAVE_KDE */

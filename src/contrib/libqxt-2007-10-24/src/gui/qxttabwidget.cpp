/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/
#include "qxttabwidget.h"
#include <QContextMenuEvent>
#include <QApplication>
#include <QTabBar>
#include <QAction>
#include <QMenu>

typedef QList<QAction*> Actions;

class QxtTabWidgetPrivate : public QxtPrivate<QxtTabWidget>
{
public:
    QXT_DECLARE_PUBLIC(QxtTabWidget);

    QxtTabWidgetPrivate();
    int tabIndexAt(const QPoint& pos) const;

    QList<Actions> actions;
    Qt::ContextMenuPolicy policy;
};

QxtTabWidgetPrivate::QxtTabWidgetPrivate() : policy(Qt::DefaultContextMenu)
{}

int QxtTabWidgetPrivate::tabIndexAt(const QPoint& pos) const
{
    const int count = qxt_p().count();
    const QTabBar* tabBar = qxt_p().tabBar();
    for (int i = 0; i < count; ++i)
        if (tabBar->tabRect(i).contains(pos))
            return i;
    return -1;
}

/*!
    \class QxtTabWidget QxtTabWidget
    \ingroup QxtGui
    \brief An extended QTabWidget.

    QxtTabWidget provides some convenience for handling tab specific context menus.

    Example usage:
    \code
    QxtTabWidget* tabWidget = new QxtTabWidget();
    tabWidget->addTab(tab0, "1");
    tabWidget->addTab(tab1, "2");

    QList<QAction*> actions0;
    actions0 << new QAction("Quisque", tab0) << new QAction("Aenean", tab0);
    QList<QAction*> actions1;
    actions1 << new QAction("Phasellus", tab1) << new QAction("Maecenas", tab1);

    tabWidget->setTabContextMenuPolicy(Qt::ActionsContextMenu);
    tabWidget->addTabActions(0, actions0);
    tabWidget->addTabActions(1, actions1);
    \endcode

    \image html qxttabwidget.png "QxtTabWidget in WindowsXP style."

    \note http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=137891
 */

/*!
    \fn QxtTabWidget::tabContextMenuRequested(int index, const QPoint& globalPos)

    This signal is emitted whenever the context menu is requested over
    tab at \a index at \a globalPos.
 */

/*!
    Constructs a new QxtTabWidget with \a parent.
 */
QxtTabWidget::QxtTabWidget(QWidget* parent) : QTabWidget(parent)
{
    QXT_INIT_PRIVATE(QxtTabWidget);
}

/*!
    Destructs the tab widget.
 */
QxtTabWidget::~QxtTabWidget()
{}

/*!
    \property QxtTabWidget::tabContextMenuPolicy
    \brief This property holds how the tab specific context menus are handled.

    The default value of this property is \b Qt::DefaultContextMenu,
    which means that the tabContextMenuEvent() handler is called.
    Other values are \b Qt::NoContextMenu, \b Qt::PreventContextMenu
    (since Qt 4.2), \b Qt::ActionsContextMenu, and \b Qt::CustomContextMenu.
    With \b Qt::CustomContextMenu, the signal tabContextMenuRequested() is
    emitted.

    \sa tabContextMenuEvent(), tabContextMenuRequested(), tabActions()
 */
Qt::ContextMenuPolicy QxtTabWidget::tabContextMenuPolicy() const
{
    return qxt_d().policy;
}

void QxtTabWidget::setTabContextMenuPolicy(Qt::ContextMenuPolicy policy)
{
    qxt_d().policy = policy;
}

/*!
    Appends the \a action to the list of actions of the
    tab at \a index.

    \sa removeTabAction(), insertTabAction(), tabActions()
 */
void QxtTabWidget::addTabAction(int index, QAction* action)
{
    insertTabAction(index, 0, action);
}

/*!
    This convenience function creates a new action with \a text. The function
    adds the newly created action to the list of actions of the tab at
    \a index, and returns it.

    \sa addTabAction()
 */
QAction* QxtTabWidget::addTabAction(int index, const QString& text)
{
    return addTabAction(index, QIcon(), text, 0, 0);
}

/*!
    This convenience function creates a new action with \a icon and \a text.
    The function adds the newly created action to the list of actions of the
    tab at \a index, and returns it.

    \sa addTabAction()
 */
QAction* QxtTabWidget::addTabAction(int index, const QIcon& icon, const QString& text)
{
    return addTabAction(index, icon, text, 0, 0);
}

/*!
    This convenience function creates a new action with \a text and
    an optional \a shortcut. The action's triggered() signal is
    connected to the \a receiver's \a member slot. The function adds
    the newly created action to the list of actions of the tab at
    \a index, and returns it.

    \note In order to make it possible for the shortcut to work even
    when the context menu is not open, the action must be added to
    a visible widget. The corresponding tab is a good alternative.

    \code
    QWidget* tab = createNewTab();
    tabWidget->addTab(tab, title);
    QAction* action = tabWidget->addTabAction(index, tr("Close"), this, SLOT(closeCurrentTab()), tr("Ctrl+W"));
    tab->addAction(act);
    \endcode

    \sa addTabAction(), QWidget::addAction()
 */
QAction* QxtTabWidget::addTabAction(int index, const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut)
{
    return addTabAction(index, QIcon(), text, receiver, member, shortcut);
}

/*!
    This convenience function creates a new action with \a icon, \a text
    and an optional \a shortcut. The action's triggered() signal is connected
    to the \a receiver's \a member slot. The function adds the newly created
    action to the list of actions of the tab at \a index, and returns it.

    \sa addTabAction()
 */
QAction* QxtTabWidget::addTabAction(int index, const QIcon& icon, const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut)
{
    QAction* action = new QAction(icon, text, this);
    addTabAction(index, action);
    if (receiver && member)
        connect(action, SIGNAL(triggered()), receiver, member);
    action->setShortcut(shortcut);
    return action;
}

/*!
    Appends the \a actions to the list of actions of the
    tab at \a index.

    \sa removeTabAction(), addTabAction()
 */
void QxtTabWidget::addTabActions(int index, QList<QAction*> actions)
{
    foreach (QAction* action, actions)
    {
        insertTabAction(index, 0, action);
    }
}

/*!
    Clears the list of actions of the tab at \a index.

    \note Only actions owned by the tab widget are deleted.

    \sa removeTabAction(), addTabAction()
 */
void QxtTabWidget::clearTabActions(int index)
{
    Q_ASSERT(index >= 0 && index < qxt_d().actions.count());

    while (qxt_d().actions[index].count())
    {
        QAction* action = qxt_d().actions[index].last();
        removeTabAction(index, action);
        if (action->parent() == this)
            delete action;
    }
}

/*!
    Inserts the \a action to the list of actions of the
    tab at \a index, before the action \a before. It appends
    the action if \a before is \c 0.

    \sa removeTabAction(), addTabAction(), tabContextMenuPolicy, tabActions()
 */
void QxtTabWidget::insertTabAction(int index, QAction* before, QAction* action)
{
    Q_ASSERT(index >= 0 && index < qxt_d().actions.count());

    if (!action)
    {
        qWarning("QxtTabWidget::insertTabAction: Attempt to insert a null action");
        return;
    }

    const Actions& actions = qxt_d().actions.at(index);
    if (actions.contains(action))
        removeTabAction(index, action);

    int pos = actions.indexOf(before);
    if (pos < 0)
    {
        before = 0;
        pos = actions.count();
    }
    qxt_d().actions[index].insert(pos, action);

    QActionEvent e(QEvent::ActionAdded, action, before);
    QApplication::sendEvent(this, &e);
}

/*!
    Inserts the \a actions to the list of actions of the
    tab at \a index, before the action \a before. It appends
    the action if \a before is \c 0.

    \sa removeAction(), QMenu, insertAction(), contextMenuPolicy
 */
void QxtTabWidget::insertTabActions(int index, QAction* before, QList<QAction*> actions)
{
    foreach (QAction* action, actions)
    {
        insertTabAction(index, before, action);
    }
}

/*!
    Removes the action \a action from the list of actions of the
    tab at \a index.

    \note The removed action is not deleted.

    \sa insertTabAction(), tabActions(), insertTabAction()
 */
void QxtTabWidget::removeTabAction(int index, QAction* action)
{
    Q_ASSERT(index >= 0 && index < qxt_d().actions.count());

    if (!action)
    {
        qWarning("QxtTabWidget::removeTabAction: Attempt to remove a null action");
        return;
    }

    if (qxt_d().actions[index].removeAll(action))
    {
        QActionEvent e(QEvent::ActionRemoved, action);
        QApplication::sendEvent(this, &e);
    }
}

/*!
    Returns the (possibly empty) list of actions for the
    tab at \a index.

    \sa tabContextMenuPolicy, insertTabAction(), removeTabAction()
 */
QList<QAction*> QxtTabWidget::tabActions(int index) const
{
    Q_ASSERT(index >= 0 && index < qxt_d().actions.count());
    return qxt_d().actions.at(index);
}

void QxtTabWidget::tabInserted(int index)
{
    Q_ASSERT(index >= 0 && index <= qxt_d().actions.count());
    qxt_d().actions.insert(index, Actions());
}

void QxtTabWidget::tabRemoved(int index)
{
    Q_ASSERT(index >= 0 && index < qxt_d().actions.count());
    qxt_d().actions.removeAt(index);
}

void QxtTabWidget::contextMenuEvent(QContextMenuEvent* event)
{
    const QPoint& pos = event->pos();
    if (!tabBar()->geometry().contains(pos))
        return QTabWidget::contextMenuEvent(event);

    const int index = qxt_d().tabIndexAt(event->pos());
    switch (qxt_d().policy)
    {
    case Qt::NoContextMenu:
        event->ignore();
        break;

#if QT_VERSION >= 0x040200
    case Qt::PreventContextMenu:
        event->accept();
        break;
#endif // QT_VERSION

    case Qt::ActionsContextMenu:
        if (index != -1 && qxt_d().actions.at(index).count())
        {
            QMenu::exec(qxt_d().actions.at(index), event->globalPos());
        }
        break;

    case Qt::CustomContextMenu:
        if (index != -1)
        {
            emit tabContextMenuRequested(index, event->globalPos());
        }
        break;

    case Qt::DefaultContextMenu:
    default:
        if (index != -1)
        {
            tabContextMenuEvent(index, event);
        }
        break;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive context menu events for the tab at \a index.

    The handler is called when \b tabContextMenuPolicy is
    \b Qt::DefaultContextMenu.

    The default implementation ignores the context menu event.

    \sa tabContextMenuPolicy, tabContextMenuRequested()
 */
void QxtTabWidget::tabContextMenuEvent(int index, QContextMenuEvent* event)
{
    Q_UNUSED(index);
    event->ignore();
}

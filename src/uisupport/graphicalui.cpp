/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This contains code from KStatusNotifierItem, part of the KDE libs     *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
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

#include "graphicalui.h"

#include "actioncollection.h"
#include "uisettings.h"
#include "contextmenuactionprovider.h"
#include "toolbaractionprovider.h"

#ifdef Q_WS_X11
#  include <QX11Info>
#endif

QWidget *GraphicalUi::_mainWidget = nullptr;
QHash<QString, ActionCollection *> GraphicalUi::_actionCollections;
ContextMenuActionProvider *GraphicalUi::_contextMenuActionProvider = nullptr;
ToolBarActionProvider *GraphicalUi::_toolBarActionProvider = nullptr;
UiStyle *GraphicalUi::_uiStyle = nullptr;


GraphicalUi::GraphicalUi(QObject *parent) : AbstractUi(parent), Singleton<GraphicalUi>(this)
{
    Q_INIT_RESOURCE(pics);
    Q_INIT_RESOURCE(hicolor_icons);
#ifdef EMBED_DATA
    Q_INIT_RESOURCE(icons);
    Q_INIT_RESOURCE(iconthemes);
#endif

#ifdef Q_OS_WIN
    _dwTickCount = 0;
#endif
#ifdef Q_OS_MAC
    GetFrontProcess(&_procNum);
#endif
}


void GraphicalUi::init()
{
#ifdef Q_OS_WIN
    mainWidget()->installEventFilter(this);
#endif
}


ActionCollection *GraphicalUi::actionCollection(const QString &category, const QString &translatedCategory)
{
    if (_actionCollections.contains(category))
        return _actionCollections.value(category);
    auto *coll = new ActionCollection(_mainWidget);

    if (!translatedCategory.isEmpty())
        coll->setProperty("Category", translatedCategory);
    else
        coll->setProperty("Category", category);

    if (_mainWidget)
        coll->addAssociatedWidget(_mainWidget);
    _actionCollections.insert(category, coll);
    return coll;
}


QHash<QString, ActionCollection *> GraphicalUi::actionCollections()
{
    return _actionCollections;
}


void GraphicalUi::loadShortcuts()
{
    foreach(ActionCollection *coll, actionCollections())
    coll->readSettings();
}


void GraphicalUi::saveShortcuts()
{
    ShortcutSettings s;
    s.clear();
    foreach(ActionCollection *coll, actionCollections())
    coll->writeSettings();
}


void GraphicalUi::setMainWidget(QWidget *widget)
{
    _mainWidget = widget;
}


void GraphicalUi::setContextMenuActionProvider(ContextMenuActionProvider *provider)
{
    _contextMenuActionProvider = provider;
}


void GraphicalUi::setToolBarActionProvider(ToolBarActionProvider *provider)
{
    _toolBarActionProvider = provider;
}


void GraphicalUi::setUiStyle(UiStyle *style)
{
    _uiStyle = style;
}


void GraphicalUi::disconnectedFromCore()
{
    _contextMenuActionProvider->disconnectedFromCore();
    _toolBarActionProvider->disconnectedFromCore();
    AbstractUi::disconnectedFromCore();
}


bool GraphicalUi::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_OS_WIN
    if (obj == mainWidget() && event->type() == QEvent::ActivationChange) {
        _dwTickCount = GetTickCount();
    }
#endif
    return AbstractUi::eventFilter(obj, event);
}


// NOTE: Window activation stuff seems to work just fine in Plasma 5 without requiring X11 hacks.
// TODO: Evaluate cleaning all this up once we can get rid of Qt4/KDE4

// Code taken from KStatusNotifierItem for handling minimize/restore

bool GraphicalUi::checkMainWidgetVisibility(bool perform)
{
#ifdef Q_OS_WIN
    // the problem is that we lose focus when the systray icon is activated
    // and we don't know the former active window
    // therefore we watch for activation event and use our stopwatch :)
    if (GetTickCount() - _dwTickCount < 300) {
        // we were active in the last 300ms -> hide it
        if (perform)
            minimizeRestore(false);
        return false;
    }
    else {
        if (perform)
            minimizeRestore(true);
        return true;
    }

#else

    if (!mainWidget()->isVisible() || mainWidget()->isMinimized() || !mainWidget()->isActiveWindow()) {
        if (perform)
            minimizeRestore(true);
        return true;
    }
    else {
        if (perform)
            minimizeRestore(false);
        return false;
    }

#endif

    return true;
}


bool GraphicalUi::isMainWidgetVisible()
{
    return !instance()->checkMainWidgetVisibility(false);
}


void GraphicalUi::minimizeRestore(bool show)
{
    if (show)
        activateMainWidget();
    else
        hideMainWidget();
}


void GraphicalUi::activateMainWidget()
{
#ifdef Q_WS_X11
    // Bypass focus stealing prevention
    QX11Info::setAppUserTime(QX11Info::appTime());
#endif

    if (mainWidget()->windowState() & Qt::WindowMinimized) {
        // restore
        mainWidget()->setWindowState((mainWidget()->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    }

    // this does not actually work on all platforms... and causes more evil than good
    // mainWidget()->move(mainWidget()->frameGeometry().topLeft()); // avoid placement policies
#ifdef Q_OS_MAC
    SetFrontProcess(&instance()->_procNum);
#else
    mainWidget()->show();
    mainWidget()->raise();
    mainWidget()->activateWindow();
#endif
}


void GraphicalUi::hideMainWidget()
{
    if (instance()->isHidingMainWidgetAllowed())
#ifdef Q_OS_MAC
        ShowHideProcess(&instance()->_procNum, false);
#else
        mainWidget()->hide();
#endif
}


void GraphicalUi::toggleMainWidget()
{
    instance()->checkMainWidgetVisibility(true);
}

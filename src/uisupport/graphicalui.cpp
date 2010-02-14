/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "graphicalui.h"

#include "actioncollection.h"
#include "contextmenuactionprovider.h"

#ifdef Q_WS_X11
#  include <QX11Info>
#endif
#ifdef HAVE_KDE
#  include <KWindowInfo>
#  include <KWindowSystem>
#endif

QWidget *GraphicalUi::_mainWidget = 0;
QHash<QString, ActionCollection *> GraphicalUi::_actionCollections;
ContextMenuActionProvider *GraphicalUi::_contextMenuActionProvider = 0;
ToolBarActionProvider *GraphicalUi::_toolBarActionProvider = 0;
UiStyle *GraphicalUi::_uiStyle = 0;
bool GraphicalUi::_onAllDesktops = false;

GraphicalUi::GraphicalUi(QObject *parent) : AbstractUi(parent)
{

}

ActionCollection *GraphicalUi::actionCollection(const QString &category) {
  if(_actionCollections.contains(category))
    return _actionCollections.value(category);
  ActionCollection *coll = new ActionCollection(_mainWidget);
  if(_mainWidget)
    coll->addAssociatedWidget(_mainWidget);
  _actionCollections.insert(category, coll);
  return coll;
}

void GraphicalUi::setMainWidget(QWidget *widget) {
  _mainWidget = widget;
}

void GraphicalUi::setContextMenuActionProvider(ContextMenuActionProvider *provider) {
  _contextMenuActionProvider = provider;
}

void GraphicalUi::setToolBarActionProvider(ToolBarActionProvider *provider) {
  _toolBarActionProvider = provider;
}

void GraphicalUi::setUiStyle(UiStyle *style) {
  _uiStyle = style;
}

void GraphicalUi::activateMainWidget() {
#ifdef HAVE_KDE
#  ifdef Q_WS_X11
    KWindowInfo info = KWindowSystem::windowInfo(mainWidget()->winId(), NET::WMDesktop | NET::WMFrameExtents);
    if(_onAllDesktops) {
      KWindowSystem::setOnAllDesktops(mainWidget()->winId(), true);
    } else {
      KWindowSystem::setCurrentDesktop(info.desktop());
    }

    mainWidget()->move(info.frameGeometry().topLeft()); // avoid placement policies
    mainWidget()->show();
    mainWidget()->raise();
    KWindowSystem::raiseWindow(mainWidget()->winId());
    KWindowSystem::activateWindow(mainWidget()->winId());
#  else
    mainWidget()->show();
    KWindowSystem::raiseWindow(mainWidget()->winId());
    KWindowSystem::forceActiveWindow(mainWidget()->winId());
#  endif

#else /* HAVE_KDE */

#ifdef Q_WS_X11
  // Bypass focus stealing prevention
  QX11Info::setAppUserTime(QX11Info::appTime());
#endif

  if(mainWidget()->windowState() & Qt::WindowMinimized) {
    // restore
    mainWidget()->setWindowState((mainWidget()->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  }

  // this does not actually work on all platforms... and causes more evil than good
  // mainWidget()->move(mainWidget()->frameGeometry().topLeft()); // avoid placement policies
  mainWidget()->show();
  mainWidget()->raise();
  mainWidget()->activateWindow();

#endif /* HAVE_KDE */
}

void GraphicalUi::hideMainWidget() {

#if defined(HAVE_KDE) && defined(Q_WS_X11)
  KWindowInfo info = KWindowSystem::windowInfo(mainWidget()->winId(), NET::WMDesktop | NET::WMFrameExtents);
  _onAllDesktops = info.onAllDesktops();
#endif

  mainWidget()->hide();
}

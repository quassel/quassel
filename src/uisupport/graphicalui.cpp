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

QWidget *GraphicalUi::_mainWidget = 0;
QHash<QString, ActionCollection *> GraphicalUi::_actionCollections;
ContextMenuActionProvider *GraphicalUi::_contextMenuActionProvider = 0;
ToolBarActionProvider *GraphicalUi::_toolBarActionProvider = 0;

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

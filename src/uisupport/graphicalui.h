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

#ifndef GRAPHICALUI_H_
#define GRAPHICALUI_H_

#include "abstractui.h"

class ActionCollection;
class ContextMenuActionProvider;
class ToolBarActionProvider;

class GraphicalUi : public AbstractUi {
  Q_OBJECT

public:
  GraphicalUi(QObject *parent = 0);

  //! Access global ActionCollections.
  /** These ActionCollections are associated with the main window, i.e. they contain global
  *  actions (and thus, shortcuts). Widgets providing application-wide shortcuts should
  *  create appropriate Action objects using GraphicalUi::actionCollection(cat)->add\<Action\>().
  *  @param category The category (default: "General")
  */
  static ActionCollection *actionCollection(const QString &category = "General");

  inline static ContextMenuActionProvider *contextMenuActionProvider();
  inline static ToolBarActionProvider *toolBarActionProvider();

protected:
  //! This is the widget we associate global actions with, typically the main window
  void setMainWidget(QWidget *);

  void setContextMenuActionProvider(ContextMenuActionProvider *);
  void setToolBarActionProvider(ToolBarActionProvider *);

private:
  static QWidget *_mainWidget;
  static QHash<QString, ActionCollection *> _actionCollections;
  static ContextMenuActionProvider *_contextMenuActionProvider;
  static ToolBarActionProvider *_toolBarActionProvider;

};

ContextMenuActionProvider *GraphicalUi::contextMenuActionProvider() {
  return _contextMenuActionProvider;
}

ToolBarActionProvider *GraphicalUi::toolBarActionProvider() {
  return _toolBarActionProvider;
}

#endif

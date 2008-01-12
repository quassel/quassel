/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "nickview.h"
#include "nickmodel.h"

#include <QHeaderView>
#include <QDebug>

NickView::NickView(QWidget *parent) : QTreeView(parent) {
  setGeometry(0, 0, 30, 30);
  //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

  setIndentation(10);
  header()->hide();
  header()->hideSection(1);
  setAnimated(true);
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

//   filteredModel = new FilteredNickModel(this);
//   QTreeView::setModel(filteredModel);
}

NickView::~NickView() {


}

// void NickView::setModel(NickModel *model) {
//   filteredModel->setSourceModel(model);
//   expandAll();
// }

void NickView::rowsInserted(const QModelIndex &index, int start, int end) {
  QTreeView::rowsInserted(index, start, end);
  expandAll();  // FIXME We need to do this more intelligently. Maybe a pimped TreeView?
}


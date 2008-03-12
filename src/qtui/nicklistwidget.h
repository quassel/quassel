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

#ifndef _NICKLISTWIDGET_H_
#define _NICKLISTWIDGET_H_

#include "ui_nicklistwidget.h"
#include "abstractitemview.h"

#include "types.h"

#include <QHash>

#include "buffermodel.h"

class Buffer;
class NickView;

class NickListWidget : public AbstractItemView {
  Q_OBJECT

public:
  NickListWidget(QWidget *parent = 0);

protected:
  virtual QSize sizeHint() const;

protected slots:
  virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
  virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

private slots:
  void removeBuffer(BufferId bufferId);
  
private:
  Ui::NickListWidget ui;
  QHash<BufferId, NickView *> nickViews;
};

#endif

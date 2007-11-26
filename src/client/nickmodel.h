/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _NICKMODEL_H_
#define _NICKMODEL_H_

#include <QAbstractItemModel>

class IrcChannel;

/*
//! Represents a single IrcUser within a NickTreeModel.
class NickTreeItem : public TreeItem {
  Q_OBJECT

  public:
    NickTreeItem(IrcUser *ircuser, TreeItem *parent = 0);

    //virtual QVariant data(int column, int row) const;

  private:

};

//! Represents a group of nicks, such as Ops, Voiced etc.
class NickTreeGroupItem : public TreeItem {
  Q_OBJECT

  public:
    NickTreeGroupItem(const QString &title, TreeItem *parent = 0);

    //virtual QVariant data(int column, int row) const;

  private:

};
*/

//! Represents the IrcUsers in a given IrcChannel.
class NickModel : public QAbstractItemModel {
  Q_OBJECT

  public:
    NickModel(IrcChannel *);
    virtual ~NickModel();

  private:
    

};

#endif

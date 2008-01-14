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

#ifndef _NICKMODEL_H_
#define _NICKMODEL_H_

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QVector>

class IrcChannel;
class IrcUser;

//! Represents the IrcUsers in a given IrcChannel.
/** This model is a wrapper around the nicks/IrcUsers stored in an IrcChannel. It provides a tree with two,
 *  levels, where the top-level items are the categories (such as Ops, Voiced etc), and the second-level items
 *  the actual nicks/users. Several roles are provided to access information about a nick.
 *
 *  Note that the nicks are not sorted in any way. Use a FilteredNickModel instead.
 */
class NickModel : public QAbstractItemModel {
  Q_OBJECT

  public:
    enum NickModelRole { SortKeyRole = Qt::UserRole };

    NickModel(IrcChannel *channel = 0, QObject *parent = 0);
    virtual ~NickModel();

    virtual QModelIndex index(int row, int col, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &) const;
    virtual int columnCount(const QModelIndex &) const;
    virtual QVariant data(const QModelIndex &, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    IrcChannel *ircChannel() const;

    QModelIndex indexOfUser(IrcUser *) const;
    int categoryFromModes(const QString &modes) const;
    int categoryFromIndex(const QModelIndex &index) const;
    int userCategory(IrcUser *) const;

  public slots:
    void setIrcChannel(IrcChannel *chan = 0);
    void addUser(IrcUser *);
    void removeUser(IrcUser *);
    void removeUser(const QModelIndex &);
    void renameUser(IrcUser *);
    void changeUserModes(IrcUser *);

  private:

    IrcChannel *_ircChannel;
    QVector<QList<IrcUser *> > users;

};

//! This ProxyModel can be used on top of a NickModel in order to provide a sorted nicklist and to hide unused categories.
class FilteredNickModel : public QSortFilterProxyModel {
  Q_OBJECT

  public:
    FilteredNickModel(QObject *parent = 0);
    virtual ~FilteredNickModel();

    virtual void setSourceModel(QAbstractItemModel *model);

  private slots:
    void sourceRowsInserted(const QModelIndex &, int, int);
    void sourceRowsRemoved(const QModelIndex &, int, int);

  protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

};

#endif

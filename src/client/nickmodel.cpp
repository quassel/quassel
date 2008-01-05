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

#include "nickmodel.h"

#include "ircchannel.h"
#include "ircuser.h"

#include <QDebug>

NickModel::NickModel(IrcChannel *channel, QObject *parent) : QAbstractItemModel(parent) {
  // we support 6 categories: q, a, o, h, v and standard
  users = QVector<QList<IrcUser *> >(6);

  if(channel) setIrcChannel(channel);
  else _ircChannel = 0;
}

NickModel::~NickModel() {


}

IrcChannel *NickModel::ircChannel() const {
  return _ircChannel;
}

void NickModel::setIrcChannel(IrcChannel *channel) {
  if(_ircChannel) {
    disconnect(_ircChannel, 0, this, 0);
  }
  foreach(QList<IrcUser *> l, users) l.clear();
  _ircChannel = channel;
  reset();
  if(_ircChannel) {
    connect(channel, SIGNAL(destroyed()), this, SLOT(setIrcChannel()));
    connect(channel, SIGNAL(ircUserJoined(IrcUser *)), this, SLOT(addUser(IrcUser *)));
    connect(channel, SIGNAL(ircUserParted(IrcUser *)), this, SLOT(removeUser(IrcUser *)));
    connect(channel, SIGNAL(ircUserNickSet(IrcUser *, QString)), this, SLOT(renameUser(IrcUser *)));
    connect(channel, SIGNAL(ircUserModesSet(IrcUser *, QString)), this, SLOT(changeUserModes(IrcUser *)));

    foreach(IrcUser *ircuser, channel->ircUsers()) {
    // TODO: make this efficient by sorting after everything is appended instead!
      addUser(ircuser);
    }
  }
}

QVariant NickModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if(section == 0 && role == Qt::DisplayRole) {
    if(ircChannel()) return ircChannel()->name();
    else return "No channel";
  }
  return QAbstractItemModel::headerData(section, orientation, role);
}

QModelIndex NickModel::index(int row, int column, const QModelIndex &parent) const {
  if(!parent.isValid()) { // Top-level item, i.e. a nick category
    if(column > 0) return QModelIndex();
    //int r = 0;
    //for(int i = 0; i < row; i++) { // we need to skip empty categories
    if(row > users.count()) {
      qDebug() << "invalid model index!";
      return QModelIndex();
    }
    return createIndex(row, column, 0);
  }
  // Second-level item, i.e. a nick. internalId() contains the parent category (starting at 1).
  int cat = parent.row() + 1;
  if(row > users[cat-1].count()) {
    qDebug() << "invalid model index!";
    return QModelIndex();
  }
  return createIndex(row, column, cat);
}

QModelIndex NickModel::indexOfUser(IrcUser *user) const {
  int idx = -1; int cat;
  for(cat = users.count()-1; cat >= 0; cat--) {
    // we count backwards, since most users will usually be in the last category
    idx = users[cat].indexOf(user);
    if(idx >=0) break;
  }
  if(idx < 0) {
    qWarning("NickModel: Index of unknown user requested!");
    return QModelIndex();
  }
  return createIndex(idx, 0, cat+1);
}

QModelIndex NickModel::parent(const QModelIndex &index) const {
  if(!index.isValid()) return QModelIndex();
  int cat = index.internalId();
  if(cat) return createIndex(cat-1, 0, 0);
  else return QModelIndex();
}

int NickModel::rowCount(const QModelIndex &parent) const {
  if(!parent.isValid()) {
    if(!ircChannel()) return 1;  // informative text
    return users.count();
  }
  int cat = parent.internalId();
  if(!cat) {  // top-level item (category)
    return users[parent.row()].count();
  }
  return 0;  // second-level items don't have children
}

int NickModel::columnCount(const QModelIndex &) const {
  //if(!ircChannel()) return 0;
  return 1;  // all our items have exactly one column
}

QVariant NickModel::data(const QModelIndex &index, int role) const {
  if(!index.isValid()) return QVariant();
  if(!ircChannel()) {
    // we show one item with informative text
    switch(role) {
      case Qt::DisplayRole: return tr("Not in channel");
      default: return QVariant();
    }
  }
  int cat = index.internalId();
  if(!cat) { // top-level item (category)
    switch(role) {
      case Qt::DisplayRole: {
        QString title;
        switch(index.row()) {
          case 0: title = tr("%n Owner(s)", "", users[index.row()].count()); break;
          case 1: title = tr("%n Admin(s)", "", users[index.row()].count()); break;
          case 2: title = tr("%n Operator(s)", "", users[index.row()].count()); break;
          case 3: title = tr("%n Half-Op(s)", "", users[index.row()].count()); break;
          case 4: title = tr("%n Voiced", "", users[index.row()].count()); break;
          case 5: title = tr("%n User(s)", "", users[index.row()].count()); break;
          default: qDebug() << "invalid model index"; return QVariant();
        }
        return title;
      }
      case SortKeyRole: return index.row();
      default: return QVariant();
    }
  } else {
    IrcUser *user = users[cat-1][index.row()];
    switch(role) {
      case Qt::DisplayRole:
        return user->nick();
      case Qt::ToolTipRole:
        return user->hostmask();
      case SortKeyRole:
        return user->nick();
      default:
        return QVariant();
    }
  }
}

int NickModel::userCategory(IrcUser *user) const {
  return categoryFromModes(ircChannel()->userModes(user));
}

int NickModel::categoryFromModes(const QString &modes) const {
  int cat;
  // we hardcode this even though we have PREFIX in networkinfo... but that wouldn't help with mapping modes to
  // category strings anyway.
  if(modes.contains('q')) cat = 1;
  else if(modes.contains('a')) cat = 2;
  else if(modes.contains('o')) cat = 3;
  else if(modes.contains('h')) cat = 4;
  else if(modes.contains('v')) cat = 5;
  else cat = 6;
  return cat;
}

int NickModel::categoryFromIndex(const QModelIndex &index) const {
  if(!index.isValid()) return -1;
  return index.internalId();
}

void NickModel::addUser(IrcUser *user) {
  int cat = userCategory(user);
  beginInsertRows(createIndex(cat-1, 0, 0), 0, 0);
  users[cat-1].prepend(user);
  endInsertRows();
}

void NickModel::removeUser(IrcUser *user) {
  // we don't know for sure which category this user was in, so we have to search
  QModelIndex index = indexOfUser(user);
  removeUser(index);
}

void NickModel::removeUser(const QModelIndex &index) {
  if(!index.isValid()) return;
  beginRemoveRows(index.parent(), index.row(), index.row());
  users[index.internalId()-1].removeAt(index.row());
  endRemoveRows();
}

void NickModel::renameUser(IrcUser *user) { //qDebug() << "renaming" << user->nick();
  QModelIndex index = indexOfUser(user);
  emit dataChanged(index, index);
}

void NickModel::changeUserModes(IrcUser *user) {
  QModelIndex oldindex = indexOfUser(user);
  if(categoryFromIndex(oldindex) == categoryFromModes(ircChannel()->userModes(user))) {
    // User is still in same category, no change necessary
    emit dataChanged(oldindex, oldindex);
  } else {
    removeUser(oldindex);
    addUser(user);
  }
}

/******************************************************************************************
 * FilteredNickModel
 ******************************************************************************************/

FilteredNickModel::FilteredNickModel(QObject *parent) : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  setSortCaseSensitivity(Qt::CaseInsensitive);
  setSortRole(NickModel::SortKeyRole);

}

FilteredNickModel::~FilteredNickModel() {

}

void FilteredNickModel::setSourceModel(QAbstractItemModel *model) {
  QSortFilterProxyModel::setSourceModel(model);
  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
  connect(model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
}

// Hide empty categories
bool FilteredNickModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  if(!source_parent.isValid()) {
    QModelIndex index = sourceModel()->index(source_row, 0);
    return sourceModel()->rowCount(index);
  }
  return true;
}

void FilteredNickModel::sourceRowsInserted(const QModelIndex &index, int start, int end) {
  if(!index.isValid()) return;
  if(sourceModel()->rowCount(index) <= end - start + 1) {
    // category no longer empty
    invalidateFilter();
  }
}

void FilteredNickModel::sourceRowsRemoved(const QModelIndex &index, int, int) {
  if(!index.isValid()) return;
  if(sourceModel()->rowCount(index) == 0) {
    // category is now empty!
    invalidateFilter();
  }
}


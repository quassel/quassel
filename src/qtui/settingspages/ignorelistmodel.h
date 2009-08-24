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

#ifndef IGNORELISTMODEL_H
#define IGNORELISTMODEL_H

#include <QAbstractItemModel>
#include <QPointer>

#include "clientignorelistmanager.h"

class IgnoreListModel : public QAbstractItemModel {
  Q_OBJECT

public:
  IgnoreListModel(QObject *parent = 0);

  virtual QVariant data(const QModelIndex &index, int role) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

  inline QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }

  inline int rowCount(const QModelIndex &parent = QModelIndex()) const;
  inline int columnCount(const QModelIndex &parent = QModelIndex()) const;

  inline bool configChanged() const { return _configChanged; }
  inline bool isReady() const { return _modelReady; }

  const IgnoreListManager::IgnoreListItem &ignoreListItemAt(int row) const;
  void setIgnoreListItemAt(int row, const IgnoreListManager::IgnoreListItem &item);
  bool newIgnoreRule(const IgnoreListManager::IgnoreListItem &item);

public slots:
  void loadDefaults();
  void removeIgnoreRule(int index);
  void revert();
  void commit();

signals:
  void configChanged(bool);
  void modelReady(bool);

private:
  ClientIgnoreListManager _clonedIgnoreListManager;
  bool _configChanged;
  bool _modelReady;

  const IgnoreListManager &ignoreListManager() const;
  IgnoreListManager &ignoreListManager();
  IgnoreListManager &cloneIgnoreListManager();

private slots:
  void clientConnected();
  void clientDisconnected();
  void initDone();
};

// Inlines
int IgnoreListModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return isReady() ? ignoreListManager().count() : 0;
}

int IgnoreListModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return isReady() ? 3 : 0;
}

#endif //IGNORELISTMODEL_H

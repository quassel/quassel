/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "aliasesmodel.h"

#include <QDebug>
#include <QStringList>

#include "client.h"
#include "signalproxy.h"

AliasesModel::AliasesModel(QObject *parent)
    : QAbstractItemModel(parent),
    _configChanged(false),
    _modelReady(false)
{
    // we need this signal for future connects to reset the data;
    connect(Client::instance(), SIGNAL(connected()), this, SLOT(clientConnected()));
    connect(Client::instance(), SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

    if (Client::isConnected())
        clientConnected();
    else
        emit modelReady(false);
}


QVariant AliasesModel::data(const QModelIndex &index, int role) const
{
    if (!_modelReady)
        return QVariant();

    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();

    switch (role) {
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return tr("<b>The shortcut for the alias</b><br />"
                      "It can be used as a regular slash command.<br /><br />"
                      "<b>Example:</b> \"foo\" can be used per /foo");
        case 1:
            return tr("<b>The string the shortcut will be expanded to</b><br />"
                      "<b>special variables:</b><br />"
                      " - <b>$i</b> represents the i'th parameter.<br />"
                      " - <b>$i..j</b> represents the i'th to j'th parameter separated by spaces.<br />"
                      " - <b>$i..</b> represents all parameters from i on separated by spaces.<br />"
                      " - <b>$i:hostname</b> represents the hostname of the user identified by the i'th parameter or a * if unknown.<br />"
                      " - <b>$0</b> the whole string.<br />"
                      " - <b>$nick</b> your current nickname<br />"
                      " - <b>$channel</b> the name of the selected channel<br /><br />"
                      "Multiple commands can be separated with semicolons<br /><br />"
                      "<b>Example:</b> \"Test $1; Test $2; Test All $0\" will be expanded to three separate messages \"Test 1\", \"Test 2\" and \"Test All 1 2 3\" when called like /test 1 2 3");
        default:
            return QVariant();
        }
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            return aliasManager()[index.row()].name;
        case 1:
            return aliasManager()[index.row()].expansion;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}


bool AliasesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!_modelReady)
        return false;

    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount() || role != Qt::EditRole)
        return false;

    QString newValue = value.toString();
    if (newValue.isEmpty())
        return false;

    switch (index.column()) {
    case 0:
        if (aliasManager().contains(newValue)) {
            return false;
        }
        else {
            cloneAliasManager()[index.row()].name = newValue;
            return true;
        }
    case 1:
        cloneAliasManager()[index.row()].expansion = newValue;
        return true;
    default:
        return false;
    }
}


void AliasesModel::newAlias()
{
    QString newName("alias");
    int i = 0;
    AliasManager &manager = cloneAliasManager();
    while (manager.contains(newName)) {
        i++;
        newName = QString("alias%1").arg(i);
    }
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    manager.addAlias(newName, "Expansion");
    endInsertRows();
}


void AliasesModel::loadDefaults()
{
    if (!_modelReady)
        return;

    AliasManager &manager = cloneAliasManager();

    if (!manager.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        for (int i = rowCount() - 1; i >= 0; i--)
            manager.removeAt(i);
        endRemoveRows();
    }

    AliasManager::AliasList defaults = AliasManager::defaults();
    beginInsertRows(QModelIndex(), 0, defaults.count() - 1);
    foreach(AliasManager::Alias alias, defaults) {
        manager.addAlias(alias.name, alias.expansion);
    }
    endInsertRows();
}


void AliasesModel::removeAlias(int index)
{
    if (index < 0 || index >= rowCount())
        return;

    AliasManager &manager = cloneAliasManager();
    beginRemoveRows(QModelIndex(), index, index);
    manager.removeAt(index);
    endRemoveRows();
}


Qt::ItemFlags AliasesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsDropEnabled;
    }
    else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }
}


QVariant AliasesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QStringList header;
    header << tr("Alias")
           << tr("Expansion");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header[section];

    return QVariant();
}


QModelIndex AliasesModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row >= rowCount() || column >= columnCount())
        return QModelIndex();

    return createIndex(row, column);
}


const AliasManager &AliasesModel::aliasManager() const
{
    if (_configChanged)
        return _clonedAliasManager;
    else
        return *Client::aliasManager();
}


AliasManager &AliasesModel::aliasManager()
{
    if (_configChanged)
        return _clonedAliasManager;
    else
        return *Client::aliasManager();
}


AliasManager &AliasesModel::cloneAliasManager()
{
    if (!_configChanged) {
        _clonedAliasManager = *Client::aliasManager();
        _configChanged = true;
        emit configChanged(true);
    }
    return _clonedAliasManager;
}


void AliasesModel::revert()
{
    if (!_configChanged)
        return;

    _configChanged = false;
    emit configChanged(false);
    reset();
}


void AliasesModel::commit()
{
    if (!_configChanged)
        return;

    Client::aliasManager()->requestUpdate(_clonedAliasManager.toVariantMap());
    revert();
}


void AliasesModel::initDone()
{
    _modelReady = true;
    reset();
    emit modelReady(true);
}


void AliasesModel::clientConnected()
{
    connect(Client::aliasManager(), SIGNAL(updated()), SLOT(revert()));
    if (Client::aliasManager()->isInitialized())
        initDone();
    else
        connect(Client::aliasManager(), SIGNAL(initDone()), SLOT(initDone()));
}


void AliasesModel::clientDisconnected()
{
    // clear
    _clonedAliasManager = ClientAliasManager();
    _modelReady = false;
    reset();
    emit modelReady(false);
}

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

#include "ignorelistmodel.h"

#include <QDebug>
#include <QStringList>
#include <QPushButton>

#include "client.h"
#include "signalproxy.h"

IgnoreListModel::IgnoreListModel(QObject *parent)
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


QVariant IgnoreListModel::data(const QModelIndex &index, int role) const
{
    if (!_modelReady)
        return QVariant();

    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();

    switch (role) {
    case Qt::ToolTipRole:
        switch (index.column()) {
        /*
      case 0: return "<b>Type:</b><br />"
    "<i><u>BySender:</u></i><br />"
      "The ignore rule is matched against the <i>nick!ident@host.mask</i> sender-string.<br />"
    "<i><u>ByMessage:</u></i><br />"
      "The ignore rule is matched against the message content.";
      case 1:
        return "<b>Strictness:</b><br />"
    "<i><u>Dynamic:</u></i><br />"
          "Messages are hidden but still get stored in the database.<br />Deactivate or delete an ignore rule to show the messages again<br />"
    "<i><u>Permanent:</u></i><br />"
           "Messages are never stored or shown anywhere.";
    */
        case 0:
            return tr("<b>Enable / Disable:</b><br />"
                      "Only enabled rules are filtered.<br />"
                      "For dynamic rules, disabling actually shows the filtered messages again");
        case 2:
            return tr("<b>Ignore rule:</b><br />"
                      "Depending on the type of the rule, the text is matched against either:<br /><br />"
                      "- <u>the message content:</u><br />"
                      "<i>Example:<i><br />"
                      "    \"*foobar*\" matches any text containing the word \"foobar\"<br /><br />"
                      "- <u>the sender string <i>nick!ident@host.name<i></u><br />"
                      "<i>Example:</i><br />"
                      "    \"*@foobar.com\" matches any sender from host foobar.com<br />"
                      "    \"stupid!.+\" (RegEx) matches any sender with nickname \"stupid\" from any host<br />");
        default:
            return QVariant();
        }
    case Qt::DisplayRole:
        switch (index.column()) {
        case 1:
            if (ignoreListManager()[index.row()].type == IgnoreListManager::SenderIgnore)
                return tr("By Sender");
            else
                return tr("By Message");
        }
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            return ignoreListManager()[index.row()].isActive;
        case 1:
            return ignoreListManager()[index.row()].type;
        case 2:
            return ignoreListManager()[index.row()].ignoreRule;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}


bool IgnoreListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!_modelReady)
        return false;

    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount() || role != Qt::EditRole)
        return false;

    QVariant newValue = value;
    if (newValue.isNull())
        return false;

    switch (index.column()) {
    case 0:
        cloneIgnoreListManager()[index.row()].isActive = newValue.toBool();
        return true;
    case 1:
        cloneIgnoreListManager()[index.row()].type = (IgnoreListManager::IgnoreType)newValue.toInt();
        return true;
    case 2:
        if (ignoreListManager().contains(newValue.toString())) {
            return false;
        }
        else {
            cloneIgnoreListManager()[index.row()].ignoreRule = newValue.toString();
            return true;
        }
    default:
        return false;
    }
}


bool IgnoreListModel::newIgnoreRule(const IgnoreListManager::IgnoreListItem &item)
{
    IgnoreListManager &manager = cloneIgnoreListManager();
    if (manager.contains(item.ignoreRule))
        return false;
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    // manager.addIgnoreListItem(item);
    manager.addIgnoreListItem(item.type, item.ignoreRule, item.isRegEx, item.strictness, item.scope,
        item.scopeRule, item.isActive);
    endInsertRows();
    return true;
}


void IgnoreListModel::loadDefaults()
{
    /*if(!_modelReady)
      return;

    IgnoreListManager &manager = cloneIgnoreListManager();

    if(!manager.isEmpty()) {
      beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
      for(int i = rowCount() - 1; i >= 0; i--)
        manager.removeAt(i);
      endRemoveRows();
    }

    IgnoreListManager::IgnoreList defaults = IgnoreListModel::defaults();
    beginInsertRows(QModelIndex(), 0, defaults.count() - 1);
    foreach(IgnoreListManager::IgnoreListItem item, defaults) {
      manager.addIgnoreListItem(item.ignoreRule, item.isRegEx, item.strictness, item.scope, item.scopeRule);
    }
    endInsertRows();*/
}


void IgnoreListModel::removeIgnoreRule(int index)
{
    if (index < 0 || index >= rowCount())
        return;

    IgnoreListManager &manager = cloneIgnoreListManager();
    beginRemoveRows(QModelIndex(), index, index);
    manager.removeAt(index);
    endRemoveRows();
}


Qt::ItemFlags IgnoreListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsDropEnabled;
    }
    else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }
}


QVariant IgnoreListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QStringList header;
    header << tr("Enabled")
           << tr("Type")
           << tr("Ignore Rule");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header[section];

    return QVariant();
}


QModelIndex IgnoreListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row >= rowCount() || column >= columnCount())
        return QModelIndex();

    return createIndex(row, column);
}


const IgnoreListManager &IgnoreListModel::ignoreListManager() const
{
    if (_configChanged)
        return _clonedIgnoreListManager;
    else
        return *Client::ignoreListManager();
}


IgnoreListManager &IgnoreListModel::ignoreListManager()
{
    if (_configChanged)
        return _clonedIgnoreListManager;
    else
        return *Client::ignoreListManager();
}


IgnoreListManager &IgnoreListModel::cloneIgnoreListManager()
{
    if (!_configChanged) {
        _clonedIgnoreListManager = *Client::ignoreListManager();
        _configChanged = true;
        emit configChanged(true);
    }
    return _clonedIgnoreListManager;
}


void IgnoreListModel::revert()
{
    if (!_configChanged)
        return;

    _configChanged = false;
    emit configChanged(false);
    reset();
}


void IgnoreListModel::commit()
{
    if (!_configChanged)
        return;

    Client::ignoreListManager()->requestUpdate(_clonedIgnoreListManager.toVariantMap());
    revert();
}


void IgnoreListModel::initDone()
{
    _modelReady = true;
    reset();
    emit modelReady(true);
}


void IgnoreListModel::clientConnected()
{
    connect(Client::ignoreListManager(), SIGNAL(updated()), SLOT(revert()));
    if (Client::ignoreListManager()->isInitialized())
        initDone();
    else
        connect(Client::ignoreListManager(), SIGNAL(initDone()), SLOT(initDone()));
}


void IgnoreListModel::clientDisconnected()
{
    // clear
    _clonedIgnoreListManager = ClientIgnoreListManager();
    _modelReady = false;
    reset();
    emit modelReady(false);
}


const IgnoreListManager::IgnoreListItem &IgnoreListModel::ignoreListItemAt(int row) const
{
    return ignoreListManager()[row];
}


// FIXME use QModelIndex?
void IgnoreListModel::setIgnoreListItemAt(int row, const IgnoreListManager::IgnoreListItem &item)
{
    cloneIgnoreListManager()[row] = item;
    emit dataChanged(createIndex(row, 0), createIndex(row, 2));
}


const QModelIndex IgnoreListModel::indexOf(const QString &rule)
{
    return createIndex(ignoreListManager().indexOf(rule), 2);
}

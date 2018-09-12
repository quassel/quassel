/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
    : QAbstractItemModel(parent)
{
    // we need this signal for future connects to reset the data;
    connect(Client::instance(), &Client::connected, this, &AliasesModel::clientConnected);
    connect(Client::instance(), &Client::disconnected, this, &AliasesModel::clientDisconnected);

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
        {
            // To avoid overwhelming the user, organize things into a table
            QString strTooltip;
            QTextStream tooltip( &strTooltip, QIODevice::WriteOnly );
            tooltip << "<qt><style>.bold { font-weight: bold; } .italic { font-style: italic; }</style>";

            // Function to add a row to the tooltip table
            auto addRow = [&](
                    const QString& key, const QString& value = QString(), bool condition = true) {
                if (condition) {
                    if (value.isEmpty()) {
                        tooltip << "<tr><td class='italic' align='left' colspan='2'>"
                                      << key << "</td></tr>";
                    } else {
                        tooltip << "<tr><td class='bold' align='left'>"
                                      << key << "</td><td>" << value << "</td></tr>";
                    }
                }
            };

            tooltip << "<p class='bold'>"
                    << tr("The string the shortcut will be expanded to") << "</p>";

            tooltip << "<p class='bold' align='center'>"
                    << tr("Special variables") << "</p>";

            // Variable option table
            tooltip << "<table cellspacing='5' cellpadding='0'>";

            // Parameter variables
            addRow(tr("Parameter variables"));
            addRow("$i", tr("i'th parameter"));
            addRow("$i..j", tr("i'th to j'th parameter separated by spaces"));
            addRow("$i..", tr("all parameters from i on separated by spaces"));

            // IrcUser handling
            addRow(tr("Nickname parameter variables"));
            addRow("$i:account",
                   tr("account of user identified by i'th parameter, or a '*' if logged out or "
                      "unknown"));
            addRow("$i:hostname",
                   tr("hostname of user identified by i'th parameter, or a '*' if unknown"));
            addRow("$i:ident",
                   tr("ident of user identified by i'th parameter, or a '*' if unknown"));
            addRow("$i:identd",
                   tr("ident of user identified by i'th parameter if verified, or a '*' if unknown "
                      "or unverified (prefixed with '~')"));

            // General variables
            addRow(tr("General variables"));
            addRow("$0", tr("the whole string"));
            addRow("$nick", tr("your current nickname"));
            addRow("$channel", tr("the name of the selected channel"));

            // End table
            tooltip << "</table>";

            // Example header
            tooltip << "<p>"
                    << tr("Multiple commands can be separated with semicolons") << "</p>";
            // Example
            tooltip << "<p>";
            tooltip << QString("<p><span class='bold'>%1</span> %2<br />").arg(
                           tr("Example:"), tr("\"Test $1; Test $2; Test All $0\""));
            tooltip << tr("...will be expanded to three separate messages \"Test 1\", \"Test 2\" "
                          "and \"Test All 1 2 3\" when called like <i>/test 1 2 3</i>")
                    << "</p>";

            // End tooltip
            tooltip << "</qt>";
            return strTooltip;
        }
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
        return {};

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
    beginResetModel();
    endResetModel();
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
    beginResetModel();
    endResetModel();
    emit modelReady(true);
}


void AliasesModel::clientConnected()
{
    connect(Client::aliasManager(), &AliasManager::updated, this, &AliasesModel::revert);
    if (Client::aliasManager()->isInitialized())
        initDone();
    else
        connect(Client::aliasManager(), &SyncableObject::initDone, this, &AliasesModel::initDone);
}


void AliasesModel::clientDisconnected()
{
    // clear
    _clonedAliasManager = ClientAliasManager();
    _modelReady = false;
    beginResetModel();
    endResetModel();
    emit modelReady(false);
}

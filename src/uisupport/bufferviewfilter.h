/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef BUFFERVIEWFILTER_H_
#define BUFFERVIEWFILTER_H_

#include <QAction>
#include <QDropEvent>
#include <QFlags>
#include <QPointer>
#include <QSet>
#include <QSortFilterProxyModel>

#include "types.h"
#include "bufferviewconfig.h"

/*****************************************
 * Buffer View Filter
 *****************************************/
class BufferViewFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum Mode {
        NoActive = 0x01,
        NoInactive = 0x02,
        SomeNets = 0x04,
        AllNets = 0x08,
        NoChannels = 0x10,
        NoQueries = 0x20,
        NoServers = 0x40,
        FullCustom = 0x80
    };
    Q_DECLARE_FLAGS(Modes, Mode)

    BufferViewFilter(QAbstractItemModel *model, BufferViewConfig *config = 0);

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    QVariant data(const QModelIndex &index, int role) const;
    QVariant checkedState(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    bool setCheckedState(const QModelIndex &index, Qt::CheckState state);

    void setConfig(BufferViewConfig *config);
    inline BufferViewConfig *config() const { return _config; }

    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QList<QAction *> actions(const QModelIndex &index);

    void setFilterString(const QString string);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
    bool bufferLessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
    bool networkLessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

signals:
    void configChanged();

private slots:
    void configInitialized();
    void enableEditMode(bool enable);
    void showServerQueriesChanged();

private:
    QPointer<BufferViewConfig> _config;
    Qt::SortOrder _sortOrder;

    bool _showServerQueries;
    bool _editMode;
    QAction _enableEditMode;
    QSet<BufferId> _toAdd;
    QSet<BufferId> _toTempRemove;
    QSet<BufferId> _toRemove;
    QString _filterString;

    bool filterAcceptBuffer(const QModelIndex &) const;
    bool filterAcceptNetwork(const QModelIndex &) const;
    void addBuffer(const BufferId &bufferId) const;
    void addBuffers(const QList<BufferId> &bufferIds) const;
    static bool bufferIdLessThan(const BufferId &, const BufferId &);
};


Q_DECLARE_OPERATORS_FOR_FLAGS(BufferViewFilter::Modes)

#endif // BUFFERVIEWFILTER_H_

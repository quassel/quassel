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

#ifndef BUFFERVIEW_H_
#define BUFFERVIEW_H_

#include <QAction>
#include <QMenu>
#include <QDockWidget>
#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QPointer>

#include "actioncollection.h"
#include "bufferviewconfig.h"
#include "networkmodel.h"
#include "treeviewtouch.h"
#include "types.h"

/*****************************************
 * The TreeView showing the Buffers
 *****************************************/
class BufferView : public TreeViewTouch
{
    Q_OBJECT

public:
    enum Direction {
        Forward = 1,
        Backward = -1
    };

    BufferView(QWidget *parent = 0);
    void init();

    void setModel(QAbstractItemModel *model);
    void setFilteredModel(QAbstractItemModel *model, BufferViewConfig *config);

    void setConfig(BufferViewConfig *config);
    inline BufferViewConfig *config() { return _config; }

    void addActionsToMenu(QMenu *menu, const QModelIndex &index);
    void addFilterActions(QMenu *contextMenu, const QModelIndex &index);

    void selectFirstBuffer();

public slots:
    void setRootIndexForNetworkId(const NetworkId &networkId);
    void removeSelectedBuffers(bool permanently = false);
    void menuActionTriggered(QAction *);
    void nextBuffer();
    void previousBuffer();
    void hideCurrentBuffer();
    void filterTextChanged(QString filterString);

signals:
    void removeBuffer(const QModelIndex &);
    void removeBufferPermanently(const QModelIndex &);

protected:
    virtual void keyPressEvent(QKeyEvent *);
    virtual void dropEvent(QDropEvent *event);
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);
    virtual void wheelEvent(QWheelEvent *);
    virtual QSize sizeHint() const;
    virtual void focusInEvent(QFocusEvent *event) { QAbstractScrollArea::focusInEvent(event); }
    virtual void contextMenuEvent(QContextMenuEvent *event);

#if QT_VERSION < 0x050000
    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
#else
    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
#endif

private slots:
    void joinChannel(const QModelIndex &index);
    void toggleHeader(bool checked);

    void storeExpandedState(const QModelIndex &networkIdx);
    void setExpandedState(const QModelIndex &networkIdx);

    void on_configChanged();
    void on_layoutChanged();

    void changeBuffer(Direction direction);

private:
    QPointer<BufferViewConfig> _config;

    enum ExpandedState {
        WasExpanded = 0x01,
        WasActive = 0x02
    };
    QHash<NetworkId, short> _expandedState;
};


// ******************************
//  BufferViewDelgate
// ******************************

class BufferViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    BufferViewDelegate(QObject *parent = 0);
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);

protected:
    virtual void customEvent(QEvent *event);
};


// ==============================
//  BufferView Dock
// ==============================
class BufferViewDock : public QDockWidget
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive STORED true)

public :
        BufferViewDock(BufferViewConfig *config, QWidget *parent);

    int bufferViewId() const;
    BufferViewConfig *config() const;
    inline BufferView *bufferView() const { return qobject_cast<BufferView *>(widget()); }
    inline bool isActive() const { return _active; }
    void setWidget(QWidget *newWidget);
    QWidget *widget() const { return _childWidget; }

    void activateFilter();

public slots:
    void setActive(bool active = true);

protected slots:
    virtual bool eventFilter(QObject *object, QEvent *event);
    virtual void focusInEvent(QFocusEvent*event) { qDebug() << event; }

private slots:
    void bufferViewRenamed(const QString &newName);
    void updateTitle();
    void configChanged();
    void onFilterReturnPressed();

private:
    QWidget *_childWidget;
    QWidget *_widget;
    QPointer<QWidget> _oldFocusItem; // QPointer in case the old item gets deleted
    QLineEdit *_filterEdit;
    bool _active;
    QString _title;
};


#endif

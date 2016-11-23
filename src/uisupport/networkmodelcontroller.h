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

#ifndef NETWORKMODELCONTROLLER_H_
#define NETWORKMODELCONTROLLER_H_

#include <QDialog>

#include "action.h"
#include "actioncollection.h"
#include "messagefilter.h"

class QComboBox;
class QDialogButtonBox;
class QLineEdit;

class NetworkModelController : public QObject
{
    Q_OBJECT

public:
    NetworkModelController(QObject *parent = 0);
    virtual ~NetworkModelController();

    // don't change enums without doublechecking masks etc. in code
    enum ActionType {
        // Network actions
        NetworkMask = 0x0f,
        NetworkConnect = 0x01,
        NetworkDisconnect = 0x02,
        NetworkConnectAllWithDropdown = 0x03,
        NetworkDisconnectAllWithDropdown = 0x04,
        NetworkConnectAll = 0x05,
        NetworkDisconnectAll = 0x06,

        // Buffer actions
        BufferMask = 0xf0,
        BufferJoin = 0x10,
        BufferPart = 0x20,
        BufferSwitchTo = 0x30,
        BufferRemove = 0x40,

        // Hide actions
        HideMask = 0x0f00,
        HideJoin = 0x0100,
        HidePart = 0x0200,
        HideQuit = 0x0300,
        HideNick = 0x0400,
        HideMode = 0x0500,
        HideDayChange = 0x0600,
        HideTopic = 0x0700,
        HideJoinPartQuit = 0xd00,
        HideUseDefaults = 0xe00,
        HideApplyToAll = 0xf00,

        // General actions
        GeneralMask = 0xf000,
        JoinChannel = 0x1000,
        ShowChannelList = 0x2000,
        ShowIgnoreList = 0x3000,
        ShowNetworkConfig = 0x4000,

        // Nick actions
        NickMask = 0xff0000,
        NickWhois = 0x010000,
        NickQuery = 0x020000,
        NickSwitchTo = 0x030000,
        NickCtcpVersion = 0x040000,
        NickCtcpPing = 0x050000,
        NickCtcpTime = 0x060000,
        NickCtcpClientinfo = 0x070000,
        NickOp = 0x080000,
        NickDeop = 0x090000,
        NickVoice = 0x0a0000,
        NickDevoice = 0x0b0000,
        NickHalfop = 0x0c0000,
        NickDehalfop = 0x0d0000,
        NickKick = 0x0e0000,
        NickBan = 0x0f0000,
        NickKickBan = 0x100000,
        NickIgnoreUser = 0x200000,
        NickIgnoreHost = 0x300000,
        NickIgnoreDomain = 0x400000,
        NickIgnoreCustom = 0x500000,
        // The next 5 types have to stay together
        // Don't change without reading ContextMenuActionProvider::addIgnoreMenu!
        NickIgnoreToggleEnabled0 = 0x600000,
        NickIgnoreToggleEnabled1 = 0x700000,
        NickIgnoreToggleEnabled2 = 0x800000,
        NickIgnoreToggleEnabled3 = 0x900000,
        NickIgnoreToggleEnabled4 = 0xa00000,

        // Actions that are handled externally
        // These emit a signal to the action requester, rather than being handled here
        ExternalMask = 0xff000000,
        HideBufferTemporarily = 0x01000000,
        HideBufferPermanently = 0x02000000
    };

    inline Action *action(ActionType type) const;

public:
    enum ItemActiveState {
        InactiveState = 0x01,
        ActiveState = 0x02
    };
    Q_DECLARE_FLAGS(ItemActiveStates, ItemActiveState)

public slots:
    virtual void connectedToCore() {}
    virtual void disconnectedFromCore() {}

protected:
    inline ActionCollection *actionCollection() const;
    inline QList<QModelIndex> indexList() const;
    inline MessageFilter *messageFilter() const;
    inline QString contextItem() const;  ///< Channel name or nick to provide context menu for
    inline QObject *receiver() const;
    inline const char *method() const;

    void setIndexList(const QModelIndex &);
    void setIndexList(const QList<QModelIndex> &);
    void setMessageFilter(MessageFilter *);
    void setContextItem(const QString &);
    void setSlot(QObject *receiver, const char *method);

    Action *registerAction(ActionType type, const QString &text, bool checkable = false);
    Action *registerAction(NetworkModelController::ActionType type, const QIcon &icon, const QString &text, bool checkable = false);
    bool checkRequirements(const QModelIndex &index, ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));

    QString nickName(const QModelIndex &index) const;
    BufferId findQueryBuffer(const QModelIndex &index, const QString &predefinedNick = QString()) const;
    BufferId findQueryBuffer(NetworkId, const QString &nickName) const;
    void removeBuffers(const QModelIndexList &indexList);

protected slots:
    virtual void actionTriggered(QAction *);

signals:
    void showChannelList(NetworkId);
    void showNetworkConfig(NetworkId);
    void showIgnoreList(QString);

protected:
    virtual void handleNetworkAction(ActionType, QAction *);
    virtual void handleBufferAction(ActionType, QAction *);
    virtual void handleHideAction(ActionType, QAction *);
    virtual void handleNickAction(ActionType, QAction *action);
    virtual void handleGeneralAction(ActionType, QAction *);
    virtual void handleExternalAction(ActionType, QAction *);

    class JoinDlg;

private:
    NetworkModel *_model;

    ActionCollection *_actionCollection;
    QHash<ActionType, Action *> _actionByType;

    QList<QModelIndex> _indexList;
    MessageFilter *_messageFilter;
    QString _contextItem; ///< Channel name or nick to provide context menu for
    QObject *_receiver;
    const char *_method;
};


//! Input dialog for joining a channel
class NetworkModelController::JoinDlg : public QDialog
{
    Q_OBJECT

public:
    JoinDlg(const QModelIndex &index, QWidget *parent = 0);

    QString channelName() const;
    QString channelPassword() const;
    NetworkId networkId() const;

private slots:
    void on_channel_textChanged(const QString &);

private:
    QComboBox *networks;
    QLineEdit *channel;
    QLineEdit *password;
    QDialogButtonBox *buttonBox;
};


// inlines
ActionCollection *NetworkModelController::actionCollection() const { return _actionCollection; }
Action *NetworkModelController::action(ActionType type) const { return _actionByType.value(type, 0); }
QList<QModelIndex> NetworkModelController::indexList() const { return _indexList; }
MessageFilter *NetworkModelController::messageFilter() const { return _messageFilter; }
QString NetworkModelController::contextItem() const { return _contextItem; }
QObject *NetworkModelController::receiver() const { return _receiver; }
const char *NetworkModelController::method() const { return _method; }

#endif

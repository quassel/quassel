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

#include <QIcon>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMap>

#include "contextmenuactionprovider.h"

#include "buffermodel.h"
#include "buffersettings.h"
#include "clientidentity.h"
#include "network.h"
#include "util.h"
#include "client.h"
#include "clientignorelistmanager.h"

ContextMenuActionProvider::ContextMenuActionProvider(QObject *parent) : NetworkModelController(parent)
{
    registerAction(NetworkConnect, QIcon::fromTheme("network-connect"), tr("Connect"));
    registerAction(NetworkDisconnect, QIcon::fromTheme("network-disconnect"), tr("Disconnect"));

    registerAction(BufferJoin, QIcon::fromTheme("irc-join-channel"), tr("Join"));
    registerAction(BufferPart, QIcon::fromTheme("irc-close-channel"), tr("Part"));
    registerAction(BufferRemove, tr("Delete Chat(s)..."));
    registerAction(BufferSwitchTo, tr("Go to Chat"));

    registerAction(HideJoinPartQuit, tr("Joins/Parts/Quits"));
    registerAction(HideJoin, tr("Joins"), true);
    registerAction(HidePart, tr("Parts"), true);
    registerAction(HideQuit, tr("Quits"), true);
    registerAction(HideNick, tr("Nick Changes"), true);
    registerAction(HideMode, tr("Mode Changes"), true);
    registerAction(HideDayChange, tr("Day Changes"), true);
    registerAction(HideTopic, tr("Topic Changes"), true);
    registerAction(HideApplyToAll, tr("Set as Default..."));
    registerAction(HideUseDefaults, tr("Use Defaults..."));

    registerAction(JoinChannel, QIcon::fromTheme("irc-join-channel"), tr("Join Channel..."));

    registerAction(NickQuery, tr("Start Query"));
    registerAction(NickSwitchTo, tr("Show Query"));
    registerAction(NickWhois, tr("Whois"));

    registerAction(NickCtcpVersion, tr("Version"));
    registerAction(NickCtcpTime, tr("Time"));
    registerAction(NickCtcpPing, tr("Ping"));
    registerAction(NickCtcpClientinfo, tr("Client info"));
    registerAction(NickIgnoreCustom, tr("Custom..."));

    // these texts are only dummies! don't think about tr() here!
    registerAction(NickIgnoreUser, "*!ident@host.domain.tld");
    registerAction(NickIgnoreHost, "*!*@host.domain.tld");
    registerAction(NickIgnoreDomain, "*!ident@*.domain.tld");
    registerAction(NickIgnoreToggleEnabled0, "Enable", true);
    registerAction(NickIgnoreToggleEnabled1, "Enable", true);
    registerAction(NickIgnoreToggleEnabled2, "Enable", true);
    registerAction(NickIgnoreToggleEnabled3, "Enable", true);
    registerAction(NickIgnoreToggleEnabled4, "Enable", true);

    registerAction(NickOp, QIcon::fromTheme("irc-operator"), tr("Give Operator Status"));
    registerAction(NickDeop, QIcon::fromTheme("irc-remove-operator"), tr("Take Operator Status"));
    registerAction(NickHalfop, QIcon::fromTheme("irc-voice"), tr("Give Half-Operator Status"));
    registerAction(NickDehalfop, QIcon::fromTheme("irc-unvoice"), tr("Take Half-Operator Status"));
    registerAction(NickVoice, QIcon::fromTheme("irc-voice"), tr("Give Voice"));
    registerAction(NickDevoice, QIcon::fromTheme("irc-unvoice"), tr("Take Voice"));
    registerAction(NickKick, QIcon::fromTheme("im-kick-user"), tr("Kick From Channel"));
    registerAction(NickBan, QIcon::fromTheme("im-ban-user"), tr("Ban From Channel"));
    registerAction(NickKickBan, QIcon::fromTheme("im-ban-kick-user"), tr("Kick && Ban"));

    registerAction(HideBufferTemporarily, tr("Hide Chat(s) Temporarily"));
    registerAction(HideBufferPermanently, tr("Hide Chat(s) Permanently"));
    registerAction(ShowChannelList, tr("Show Channel List"));
    registerAction(ShowNetworkConfig, tr("Configure"));
    registerAction(ShowIgnoreList, tr("Show Ignore List"));

    QMenu *hideEventsMenu = new QMenu();
    hideEventsMenu->addAction(action(HideJoinPartQuit));
    hideEventsMenu->addSeparator();
    hideEventsMenu->addAction(action(HideJoin));
    hideEventsMenu->addAction(action(HidePart));
    hideEventsMenu->addAction(action(HideQuit));
    hideEventsMenu->addAction(action(HideNick));
    hideEventsMenu->addAction(action(HideMode));
    hideEventsMenu->addAction(action(HideTopic));
    hideEventsMenu->addAction(action(HideDayChange));
    hideEventsMenu->addSeparator();
    hideEventsMenu->addAction(action(HideApplyToAll));
    hideEventsMenu->addAction(action(HideUseDefaults));
    _hideEventsMenuAction = new Action(tr("Hide Events"), 0);
    _hideEventsMenuAction->setMenu(hideEventsMenu);

    QMenu *nickCtcpMenu = new QMenu();
    nickCtcpMenu->addAction(action(NickCtcpPing));
    nickCtcpMenu->addAction(action(NickCtcpVersion));
    nickCtcpMenu->addAction(action(NickCtcpTime));
    nickCtcpMenu->addAction(action(NickCtcpClientinfo));
    _nickCtcpMenuAction = new Action(tr("CTCP"), 0);
    _nickCtcpMenuAction->setMenu(nickCtcpMenu);

    QMenu *nickModeMenu = new QMenu();
    nickModeMenu->addAction(action(NickOp));
    nickModeMenu->addAction(action(NickDeop));
    // this is where the halfops will be placed if available
    nickModeMenu->addAction(action(NickHalfop));
    nickModeMenu->addAction(action(NickDehalfop));
    nickModeMenu->addAction(action(NickVoice));
    nickModeMenu->addAction(action(NickDevoice));
    nickModeMenu->addSeparator();
    nickModeMenu->addAction(action(NickKick));
    nickModeMenu->addAction(action(NickBan));
    nickModeMenu->addAction(action(NickKickBan));
    _nickModeMenuAction = new Action(tr("Actions"), 0);
    _nickModeMenuAction->setMenu(nickModeMenu);

    QMenu *ignoreMenu = new QMenu();
    _nickIgnoreMenuAction = new Action(tr("Ignore"), 0);
    _nickIgnoreMenuAction->setMenu(ignoreMenu);

    // These are disabled actions used as descriptions
    // They don't need any of the Action fancyness so we use plain QActions
    _ignoreDescriptions << new QAction(tr("Add Ignore Rule"), this);
    _ignoreDescriptions << new QAction(tr("Existing Rules"), this);
    foreach(QAction *act, _ignoreDescriptions)
    act->setEnabled(false);
}


ContextMenuActionProvider::~ContextMenuActionProvider()
{
    _hideEventsMenuAction->menu()->deleteLater();
    _hideEventsMenuAction->deleteLater();
    _nickCtcpMenuAction->menu()->deleteLater();
    _nickCtcpMenuAction->deleteLater();
    _nickModeMenuAction->menu()->deleteLater();
    _nickModeMenuAction->deleteLater();
    _nickIgnoreMenuAction->menu()->deleteLater();
    _nickIgnoreMenuAction->deleteLater();
    qDeleteAll(_ignoreDescriptions);
    _ignoreDescriptions.clear();
}


void ContextMenuActionProvider::addActions(QMenu *menu, BufferId bufId, QObject *receiver, const char *method)
{
    if (!bufId.isValid())
        return;
    addActions(menu, Client::networkModel()->bufferIndex(bufId), receiver, method);
}


void ContextMenuActionProvider::addActions(QMenu *menu, const QModelIndex &index, QObject *receiver, const char *method, bool isCustomBufferView)
{
    if (!index.isValid())
        return;
    addActions(menu, QList<QModelIndex>() << index, 0, QString(), receiver, method, isCustomBufferView);
}


void ContextMenuActionProvider::addActions(QMenu *menu, MessageFilter *filter, BufferId msgBuffer, QObject *receiver, const char *slot)
{
    addActions(menu, filter, msgBuffer, QString(), receiver, slot);
}


void ContextMenuActionProvider::addActions(QMenu *menu, MessageFilter *filter, BufferId msgBuffer, const QString &chanOrNick, QObject *receiver, const char *method)
{
    if (!filter)
        return;
    addActions(menu, QList<QModelIndex>() << Client::networkModel()->bufferIndex(msgBuffer), filter, chanOrNick, receiver, method, false);
}


void ContextMenuActionProvider::addActions(QMenu *menu, const QList<QModelIndex> &indexList, QObject *receiver,  const char *method, bool isCustomBufferView)
{
    addActions(menu, indexList, 0, QString(), receiver, method, isCustomBufferView);
}


// add a list of actions sensible for the current item(s)
void ContextMenuActionProvider::addActions(QMenu *menu,
    const QList<QModelIndex> &indexList_,
    MessageFilter *filter_,
    const QString &contextItem_,
    QObject *receiver_,
    const char *method_,
    bool isCustomBufferView)
{
    if (!indexList_.count())
        return;

    setIndexList(indexList_);
    setMessageFilter(filter_);
    setContextItem(contextItem_);
    setSlot(receiver_, method_);

    if (!messageFilter()) {
        // this means we are in a BufferView (or NickView) rather than a ChatView

        // first index in list determines the menu type (just in case we have both buffers and networks selected, for example)
        QModelIndex index = indexList().at(0);
        NetworkModel::ItemType itemType = static_cast<NetworkModel::ItemType>(index.data(NetworkModel::ItemTypeRole).toInt());

        switch (itemType) {
        case NetworkModel::NetworkItemType:
            addNetworkItemActions(menu, index);
            break;
        case NetworkModel::BufferItemType:
            addBufferItemActions(menu, index, isCustomBufferView);
            break;
        case NetworkModel::IrcUserItemType:
            addIrcUserActions(menu, index);
            break;
        default:
            return;
        }
    }
    else {
        // ChatView actions
        if (contextItem().isEmpty()) {
            // a) query buffer: handle like ircuser
            // b) general chatview: handle like channel iff it displays a single buffer
            // NOTE stuff breaks probably with merged buffers, need to rework a lot around here then
            if (messageFilter()->containedBuffers().count() == 1) {
                // we can handle this like a single bufferItem
                QModelIndex index = Client::networkModel()->bufferIndex(messageFilter()->containedBuffers().values().at(0));
                setIndexList(index);
                addBufferItemActions(menu, index);
                return;
            }
            else {
                // TODO: actions for merged buffers... _indexList contains the index of the message we clicked on
            }
        }
        else {
            // context item = chan or nick, _indexList = buf where the msg clicked on originated
            if (isChannelName(contextItem())) {
                QModelIndex msgIdx = indexList().at(0);
                if (!msgIdx.isValid())
                    return;
                NetworkId networkId = msgIdx.data(NetworkModel::NetworkIdRole).value<NetworkId>();
                BufferId bufId = Client::networkModel()->bufferId(networkId, contextItem());
                if (bufId.isValid()) {
                    QModelIndex targetIdx = Client::networkModel()->bufferIndex(bufId);
                    setIndexList(targetIdx);
                    addAction(BufferJoin, menu, targetIdx, InactiveState);
                    addAction(BufferSwitchTo, menu, targetIdx, ActiveState);
                }
                else
                    addAction(JoinChannel, menu);
            }
            else {
                // TODO: actions for a nick
            }
        }
    }
}


void ContextMenuActionProvider::addNetworkItemActions(QMenu *menu, const QModelIndex &index)
{
    NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    if (!networkId.isValid())
        return;
    const Network *network = Client::network(networkId);
    Q_CHECK_PTR(network);
    if (!network)
        return;

    addAction(ShowNetworkConfig, menu, index);
    menu->addSeparator();
    addAction(NetworkConnect, menu, network->connectionState() == Network::Disconnected);
    addAction(NetworkDisconnect, menu, network->connectionState() != Network::Disconnected);
    menu->addSeparator();
    addAction(ShowChannelList, menu, index, ActiveState);
    addAction(JoinChannel, menu, index, ActiveState);
}


void ContextMenuActionProvider::addBufferItemActions(QMenu *menu, const QModelIndex &index, bool isCustomBufferView)
{
    BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();

    menu->addSeparator();
    switch (bufferInfo.type()) {
    case BufferInfo::ChannelBuffer:
        addAction(BufferJoin, menu, index, InactiveState);
        addAction(BufferPart, menu, index, ActiveState);
        menu->addSeparator();
        addHideEventsMenu(menu, bufferInfo.bufferId());
        menu->addSeparator();
        addAction(HideBufferTemporarily, menu, isCustomBufferView);
        addAction(HideBufferPermanently, menu, isCustomBufferView);
        addAction(BufferRemove, menu, index, InactiveState);
        break;

    case BufferInfo::QueryBuffer:
    {
        //IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
        //if(ircUser) {
        addIrcUserActions(menu, index);
        menu->addSeparator();
        //}
        addHideEventsMenu(menu, bufferInfo.bufferId());
        menu->addSeparator();
        addAction(HideBufferTemporarily, menu, isCustomBufferView);
        addAction(HideBufferPermanently, menu, isCustomBufferView);
        addAction(BufferRemove, menu, index);
        break;
    }

    default:
        addAction(HideBufferTemporarily, menu, isCustomBufferView);
        addAction(HideBufferPermanently, menu, isCustomBufferView);
    }
}


void ContextMenuActionProvider::addIrcUserActions(QMenu *menu, const QModelIndex &index)
{
    // this can be called: a) as a nicklist context menu (index has IrcUserItemType)
    //                     b) as a query buffer context menu (index has BufferItemType and is a QueryBufferItem)
    //                     c) right-click in a query chatview (same as b), index will be the corresponding QueryBufferItem)
    //                     d) right-click on some nickname (_contextItem will be non-null, _filter -> chatview, index -> message buffer)

    if (contextItem().isNull()) {
        // cases a, b, c
        bool haveQuery = indexList().count() == 1 && findQueryBuffer(index).isValid();
        NetworkModel::ItemType itemType = static_cast<NetworkModel::ItemType>(index.data(NetworkModel::ItemTypeRole).toInt());
        addAction(_nickModeMenuAction, menu, itemType == NetworkModel::IrcUserItemType);
        addAction(_nickCtcpMenuAction, menu);

        IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
        if (ircUser) {
            Network *network = ircUser->network();
            // only show entries for usermode +h if server supports it
            if (network && network->prefixModes().contains('h')) {
                action(NickHalfop)->setVisible(true);
                action(NickDehalfop)->setVisible(true);
            }
            else {
                action(NickHalfop)->setVisible(false);
                action(NickDehalfop)->setVisible(false);
            }
            // ignoreliststuff
            QString bufferName;
            BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
            if (bufferInfo.type() == BufferInfo::ChannelBuffer)
                bufferName = bufferInfo.bufferName();
            QMap<QString, bool> ignoreMap = Client::ignoreListManager()->matchingRulesForHostmask(ircUser->hostmask(), ircUser->network()->networkName(), bufferName);
            addIgnoreMenu(menu, ircUser->hostmask(), ignoreMap);
            // end of ignoreliststuff
        }
        menu->addSeparator();
        addAction(NickQuery, menu, itemType == NetworkModel::IrcUserItemType && !haveQuery && indexList().count() == 1);
        addAction(NickSwitchTo, menu, itemType == NetworkModel::IrcUserItemType && haveQuery);
        menu->addSeparator();
        addAction(NickWhois, menu, true);
    }
    else if (!contextItem().isEmpty() && messageFilter()) {
        // case d
        // TODO
    }
}


Action *ContextMenuActionProvider::addAction(ActionType type, QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState)
{
    return addAction(action(type), menu, checkRequirements(index, requiredActiveState));
}


Action *ContextMenuActionProvider::addAction(Action *action, QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState)
{
    return addAction(action, menu, checkRequirements(index, requiredActiveState));
}


Action *ContextMenuActionProvider::addAction(ActionType type, QMenu *menu, bool condition)
{
    return addAction(action(type), menu, condition);
}


Action *ContextMenuActionProvider::addAction(Action *action, QMenu *menu, bool condition)
{
    if (condition) {
        menu->addAction(action);
        action->setVisible(true);
    }
    else {
        action->setVisible(false);
    }
    return action;
}


void ContextMenuActionProvider::addHideEventsMenu(QMenu *menu, BufferId bufferId)
{
    if (BufferSettings(bufferId).hasFilter())
        addHideEventsMenu(menu, BufferSettings(bufferId).messageFilter());
    else
        addHideEventsMenu(menu);
}


void ContextMenuActionProvider::addHideEventsMenu(QMenu *menu, MessageFilter *msgFilter)
{
    if (BufferSettings(msgFilter->idString()).hasFilter())
        addHideEventsMenu(menu, BufferSettings(msgFilter->idString()).messageFilter());
    else
        addHideEventsMenu(menu);
}


void ContextMenuActionProvider::addHideEventsMenu(QMenu *menu, int filter)
{
    action(HideApplyToAll)->setEnabled(filter != -1);
    action(HideUseDefaults)->setEnabled(filter != -1);
    if (filter == -1)
        filter = BufferSettings().messageFilter();

    action(HideJoin)->setChecked(filter & Message::Join);
    action(HidePart)->setChecked(filter & Message::Part);
    action(HideQuit)->setChecked(filter & Message::Quit);
    action(HideNick)->setChecked(filter & Message::Nick);
    action(HideMode)->setChecked(filter & Message::Mode);
    action(HideDayChange)->setChecked(filter & Message::DayChange);
    action(HideTopic)->setChecked(filter & Message::Topic);

    menu->addAction(_hideEventsMenuAction);
}


void ContextMenuActionProvider::addIgnoreMenu(QMenu *menu, const QString &hostmask, const QMap<QString, bool> &ignoreMap)
{
    QMenu *ignoreMenu = _nickIgnoreMenuAction->menu();
    ignoreMenu->clear();
    QString nick = nickFromMask(hostmask);
    QString ident = userFromMask(hostmask);
    QString host = hostFromMask(hostmask);
    QString domain = host;
    QRegExp domainRx = QRegExp("(\\.[^.]+\\.\\w+\\D)$");
    if (domainRx.indexIn(host) != -1)
        domain = domainRx.cap(1);
    // we can't rely on who-data
    // if we don't have the data, we skip actions where we would need it
    bool haveWhoData = !ident.isEmpty() && !host.isEmpty();

    // add "Add Ignore Rule" description
    ignoreMenu->addAction(_ignoreDescriptions.at(0));

    if (haveWhoData) {
        QString text;
        text = QString("*!%1@%2").arg(ident, host);
        action(NickIgnoreUser)->setText(text);
        action(NickIgnoreUser)->setProperty("ignoreRule", text);

        text = QString("*!*@%1").arg(host);
        action(NickIgnoreHost)->setText(text);
        action(NickIgnoreHost)->setProperty("ignoreRule", text);

        text = domain.at(0) == '.' ? QString("*!%1@*%2").arg(ident, domain)
               : QString("*!%1@%2").arg(ident, domain);

        action(NickIgnoreDomain)->setText(text);
        action(NickIgnoreDomain)->setProperty("ignoreRule", text);

        if (!ignoreMap.contains(action(NickIgnoreUser)->property("ignoreRule").toString()))
            ignoreMenu->addAction(action(NickIgnoreUser));
        if (!ignoreMap.contains(action(NickIgnoreHost)->property("ignoreRule").toString()))
            ignoreMenu->addAction(action(NickIgnoreHost));
        // we only add that NickIgnoreDomain if it isn't the same as NickIgnoreUser
        // as happens with @foobar.com hostmasks and ips
        if (!ignoreMap.contains(action(NickIgnoreDomain)->property("ignoreRule").toString())
            && action(NickIgnoreUser)->property("ignoreRule").toString() != action(NickIgnoreDomain)->property("ignoreRule").toString())
            ignoreMenu->addAction(action(NickIgnoreDomain));
    }

    action(NickIgnoreCustom)->setProperty("ignoreRule", hostmask);
    ignoreMenu->addAction(action(NickIgnoreCustom));

    ignoreMenu->addSeparator();

    if (haveWhoData) {
        QMap<QString, bool>::const_iterator ruleIter = ignoreMap.begin();
        int counter = 0;
        if (!ignoreMap.isEmpty())
            // add "Existing Rules" description
            ignoreMenu->addAction(_ignoreDescriptions.at(1));
        while (ruleIter != ignoreMap.constEnd()) {
            if (counter < 5) {
                ActionType type = static_cast<ActionType>(NickIgnoreToggleEnabled0 + counter*0x100000);
                Action *act = action(type);
                act->setText(ruleIter.key());
                act->setProperty("ignoreRule", ruleIter.key());
                act->setChecked(ruleIter.value());
                ignoreMenu->addAction(act);
            }
            counter++;
            ++ruleIter;
        }
        if (counter)
            ignoreMenu->addSeparator();
    }
    ignoreMenu->addAction(action(ShowIgnoreList));
    addAction(_nickIgnoreMenuAction, menu);
}

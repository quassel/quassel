/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

#include "networkmodelcontroller.h"

#include "buffermodel.h"
#include "buffersettings.h"
#include "clientidentity.h"
#include "network.h"
#include "util.h"
#include "clientignorelistmanager.h"
#include "client.h"

NetworkModelController::NetworkModelController(QObject *parent)
    : QObject(parent),
    _actionCollection(new ActionCollection(this)),
    _messageFilter(0),
    _receiver(0)
{
    connect(_actionCollection, SIGNAL(actionTriggered(QAction *)), SLOT(actionTriggered(QAction *)));
}


NetworkModelController::~NetworkModelController()
{
}


Action *NetworkModelController::registerAction(ActionType type, const QString &text, bool checkable)
{
    return registerAction(type, QPixmap(), text, checkable);
}


Action *NetworkModelController::registerAction(ActionType type, const QIcon &icon, const QString &text, bool checkable)
{
    Action *act;
    if (icon.isNull())
        act = new Action(text, this);
    else
        act = new Action(icon, text, this);

    act->setCheckable(checkable);
    act->setData(type);

    _actionCollection->addAction(QString::number(type, 16), act);
    _actionByType[type] = act;
    return act;
}


/******** Helper Functions ***********************************************************************/

void NetworkModelController::setIndexList(const QModelIndex &index)
{
    _indexList = QList<QModelIndex>() << index;
}


void NetworkModelController::setIndexList(const QList<QModelIndex> &list)
{
    _indexList = list;
}


void NetworkModelController::setMessageFilter(MessageFilter *filter)
{
    _messageFilter = filter;
}


void NetworkModelController::setContextItem(const QString &contextItem)
{
    _contextItem = contextItem;
}


void NetworkModelController::setSlot(QObject *receiver, const char *method)
{
    _receiver = receiver;
    _method = method;
}


bool NetworkModelController::checkRequirements(const QModelIndex &index, ItemActiveStates requiredActiveState)
{
    if (!index.isValid())
        return false;

    ItemActiveStates isActive = index.data(NetworkModel::ItemActiveRole).toBool()
                                ? ActiveState
                                : InactiveState;

    if (!(isActive & requiredActiveState))
        return false;

    return true;
}


QString NetworkModelController::nickName(const QModelIndex &index) const
{
    IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
    if (ircUser)
        return ircUser->nick();

    BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    if (!bufferInfo.isValid())
        return QString();
    if (bufferInfo.type() != BufferInfo::QueryBuffer)
        return QString();

    return bufferInfo.bufferName(); // FIXME this might break with merged queries maybe
}


BufferId NetworkModelController::findQueryBuffer(const QModelIndex &index, const QString &predefinedNick) const
{
    NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    if (!networkId.isValid())
        return BufferId();

    QString nick = predefinedNick.isEmpty() ? nickName(index) : predefinedNick;
    if (nick.isEmpty())
        return BufferId();

    return findQueryBuffer(networkId, nick);
}


BufferId NetworkModelController::findQueryBuffer(NetworkId networkId, const QString &nick) const
{
    return Client::networkModel()->bufferId(networkId, nick);
}


void NetworkModelController::removeBuffers(const QModelIndexList &indexList)
{
    QList<BufferInfo> inactive;
    foreach(QModelIndex index, indexList) {
        BufferInfo info = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
        if (info.isValid()) {
            if (info.type() == BufferInfo::QueryBuffer
                || (info.type() == BufferInfo::ChannelBuffer && !index.data(NetworkModel::ItemActiveRole).toBool()))
                inactive << info;
        }
    }
    QString msg;
    if (inactive.count()) {
        msg = tr("Do you want to delete the following buffer(s) permanently?", 0, inactive.count());
        msg += "<ul>";
        int count = 0;
        foreach(BufferInfo info, inactive) {
            if (count < 10) {
                msg += QString("<li>%1</li>").arg(info.bufferName());
                count++;
            }
            else
                break;
        }
        msg += "</ul>";
        if (count > 9 && inactive.size() - count != 0)
            msg += tr("...and <b>%1</b> more<br><br>").arg(inactive.size() - count);
        msg += tr("<b>Note:</b> This will delete all related data, including all backlog data, from the core's database and cannot be undone.");
        if (inactive.count() != indexList.count())
            msg += tr("<br>Active channel buffers cannot be deleted, please part the channel first.");

        if (QMessageBox::question(0, tr("Remove buffers permanently?"), msg, QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            foreach(BufferInfo info, inactive)
            Client::removeBuffer(info.bufferId());
        }
    }
}


void NetworkModelController::handleExternalAction(ActionType type, QAction *action)
{
    Q_UNUSED(type);
    if (receiver() && method()) {
        if (!QMetaObject::invokeMethod(receiver(), method(), Q_ARG(QAction *, action)))
            qWarning() << "NetworkModelActionController::handleExternalAction(): Could not invoke slot" << receiver() << method();
    }
}


/******** Handle Actions *************************************************************************/

void NetworkModelController::actionTriggered(QAction *action)
{
    ActionType type = (ActionType)action->data().toInt();
    if (type > 0) {
        if (type & NetworkMask)
            handleNetworkAction(type, action);
        else if (type & BufferMask)
            handleBufferAction(type, action);
        else if (type & HideMask)
            handleHideAction(type, action);
        else if (type & GeneralMask)
            handleGeneralAction(type, action);
        else if (type & NickMask)
            handleNickAction(type, action);
        else if (type & ExternalMask)
            handleExternalAction(type, action);
        else
            qWarning() << "NetworkModelController::actionTriggered(): Unhandled action!";
    }
}


void NetworkModelController::handleNetworkAction(ActionType type, QAction *)
{
    if (type == NetworkConnectAllWithDropdown || type == NetworkDisconnectAllWithDropdown || type == NetworkConnectAll || type == NetworkDisconnectAll) {
        foreach(NetworkId id, Client::networkIds()) {
            const Network *net = Client::network(id);
            if ((type == NetworkConnectAllWithDropdown || type == NetworkConnectAll) && net->connectionState() == Network::Disconnected)
                net->requestConnect();
            if ((type == NetworkDisconnectAllWithDropdown || type == NetworkDisconnectAll) && net->connectionState() != Network::Disconnected)
                net->requestDisconnect();
        }
        return;
    }

    if (!indexList().count())
        return;

    const Network *network = Client::network(indexList().at(0).data(NetworkModel::NetworkIdRole).value<NetworkId>());
    Q_CHECK_PTR(network);
    if (!network)
        return;

    switch (type) {
    case NetworkConnect:
        network->requestConnect();
        break;
    case NetworkDisconnect:
        network->requestDisconnect();
        break;
    default:
        break;
    }
}


void NetworkModelController::handleBufferAction(ActionType type, QAction *)
{
    if (type == BufferRemove) {
        removeBuffers(indexList());
    }
    else {
        QList<BufferInfo> bufferList; // create temp list because model indexes might change
        foreach(QModelIndex index, indexList()) {
            BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
            if (bufferInfo.isValid())
                bufferList << bufferInfo;
        }

        foreach(BufferInfo bufferInfo, bufferList) {
            switch (type) {
            case BufferJoin:
                Client::userInput(bufferInfo, QString("/JOIN %1").arg(bufferInfo.bufferName()));
                break;
            case BufferPart:
            {
                QString reason = Client::identity(Client::network(bufferInfo.networkId())->identity())->partReason();
                Client::userInput(bufferInfo, QString("/PART %1").arg(reason));
                break;
            }
            case BufferSwitchTo:
                Client::bufferModel()->switchToBuffer(bufferInfo.bufferId());
                break;
            default:
                break;
            }
        }
    }
}


void NetworkModelController::handleHideAction(ActionType type, QAction *action)
{
    Q_UNUSED(action)

    if (type == HideJoinPartQuit) {
        bool anyChecked = NetworkModelController::action(HideJoin)->isChecked();
        anyChecked |= NetworkModelController::action(HidePart)->isChecked();
        anyChecked |= NetworkModelController::action(HideQuit)->isChecked();

        // If any are checked, uncheck them all.
        // If none are checked, check them all.
        bool newCheckedState = !anyChecked;
        NetworkModelController::action(HideJoin)->setChecked(newCheckedState);
        NetworkModelController::action(HidePart)->setChecked(newCheckedState);
        NetworkModelController::action(HideQuit)->setChecked(newCheckedState);
    }

    int filter = 0;
    if (NetworkModelController::action(HideJoin)->isChecked())
        filter |= Message::Join | Message::NetsplitJoin;
    if (NetworkModelController::action(HidePart)->isChecked())
        filter |= Message::Part;
    if (NetworkModelController::action(HideQuit)->isChecked())
        filter |= Message::Quit | Message::NetsplitQuit;
    if (NetworkModelController::action(HideNick)->isChecked())
        filter |= Message::Nick;
    if (NetworkModelController::action(HideMode)->isChecked())
        filter |= Message::Mode;
    if (NetworkModelController::action(HideDayChange)->isChecked())
        filter |= Message::DayChange;
    if (NetworkModelController::action(HideTopic)->isChecked())
        filter |= Message::Topic;

    switch (type) {
    case HideJoinPartQuit:
    case HideJoin:
    case HidePart:
    case HideQuit:
    case HideNick:
    case HideMode:
    case HideDayChange:
    case HideTopic:
        if (_messageFilter)
            BufferSettings(_messageFilter->idString()).setMessageFilter(filter);
        else {
            foreach(QModelIndex index, _indexList) {
                BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
                if (!bufferId.isValid())
                    continue;
                BufferSettings(bufferId).setMessageFilter(filter);
            }
        }
        return;
    case HideApplyToAll:
        BufferSettings().setMessageFilter(filter);
    case HideUseDefaults:
        if (_messageFilter)
            BufferSettings(_messageFilter->idString()).removeFilter();
        else {
            foreach(QModelIndex index, _indexList) {
                BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
                if (!bufferId.isValid())
                    continue;
                BufferSettings(bufferId).removeFilter();
            }
        }
        return;
    default:
        return;
    };
}


void NetworkModelController::handleGeneralAction(ActionType type, QAction *action)
{
    Q_UNUSED(action)

    if (!indexList().count())
        return;
    NetworkId networkId = indexList().at(0).data(NetworkModel::NetworkIdRole).value<NetworkId>();

    switch (type) {
    case JoinChannel:
    {
        QString channelName = contextItem();
        QString channelPassword;
        if (channelName.isEmpty()) {
            JoinDlg dlg(indexList().first());
            if (dlg.exec() == QDialog::Accepted) {
                channelName = dlg.channelName();
                networkId = dlg.networkId();
                channelPassword = dlg.channelPassword();
            }
        }
        if (!channelName.isEmpty()) {
            if (!channelPassword.isEmpty())
                Client::instance()->userInput(BufferInfo::fakeStatusBuffer(networkId), QString("/JOIN %1 %2").arg(channelName).arg(channelPassword));
            else
                Client::instance()->userInput(BufferInfo::fakeStatusBuffer(networkId), QString("/JOIN %1").arg(channelName));
        }
        break;
    }
    case ShowChannelList:
        if (networkId.isValid())
            emit showChannelList(networkId);
        break;
    case ShowIgnoreList:
        if (networkId.isValid())
            emit showIgnoreList(QString());
        break;
    default:
        break;
    }
}


void NetworkModelController::handleNickAction(ActionType type, QAction *action)
{
    foreach(QModelIndex index, indexList()) {
        NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
        if (!networkId.isValid())
            continue;
        QString nick = nickName(index);
        if (nick.isEmpty())
            continue;
        BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
        if (!bufferInfo.isValid())
            continue;

        switch (type) {
        case NickWhois:
            Client::userInput(bufferInfo, QString("/WHOIS %1 %1").arg(nick));
            break;
        case NickCtcpVersion:
            Client::userInput(bufferInfo, QString("/CTCP %1 VERSION").arg(nick));
            break;
        case NickCtcpPing:
            Client::userInput(bufferInfo, QString("/CTCP %1 PING").arg(nick));
            break;
        case NickCtcpTime:
            Client::userInput(bufferInfo, QString("/CTCP %1 TIME").arg(nick));
            break;
        case NickCtcpClientinfo:
            Client::userInput(bufferInfo, QString("/CTCP %1 CLIENTINFO").arg(nick));
            break;
        case NickOp:
            Client::userInput(bufferInfo, QString("/OP %1").arg(nick));
            break;
        case NickDeop:
            Client::userInput(bufferInfo, QString("/DEOP %1").arg(nick));
            break;
        case NickHalfop:
            Client::userInput(bufferInfo, QString("/HALFOP %1").arg(nick));
            break;
        case NickDehalfop:
            Client::userInput(bufferInfo, QString("/DEHALFOP %1").arg(nick));
            break;
        case NickVoice:
            Client::userInput(bufferInfo, QString("/VOICE %1").arg(nick));
            break;
        case NickDevoice:
            Client::userInput(bufferInfo, QString("/DEVOICE %1").arg(nick));
            break;
        case NickKick:
            Client::userInput(bufferInfo, QString("/KICK %1").arg(nick));
            break;
        case NickBan:
            Client::userInput(bufferInfo, QString("/BAN %1").arg(nick));
            break;
        case NickKickBan:
            Client::userInput(bufferInfo, QString("/BAN %1").arg(nick));
            Client::userInput(bufferInfo, QString("/KICK %1").arg(nick));
            break;
        case NickSwitchTo:
        case NickQuery:
            Client::bufferModel()->switchToOrStartQuery(networkId, nick);
            break;
        case NickIgnoreUser:
        {
            IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
            if (!ircUser)
                break;
            Client::ignoreListManager()->requestAddIgnoreListItem(IgnoreListManager::SenderIgnore,
                action->property("ignoreRule").toString(),
                false, IgnoreListManager::SoftStrictness,
                IgnoreListManager::NetworkScope,
                ircUser->network()->networkName(), true);
            break;
        }
        case NickIgnoreHost:
        {
            IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
            if (!ircUser)
                break;
            Client::ignoreListManager()->requestAddIgnoreListItem(IgnoreListManager::SenderIgnore,
                action->property("ignoreRule").toString(),
                false, IgnoreListManager::SoftStrictness,
                IgnoreListManager::NetworkScope,
                ircUser->network()->networkName(), true);
            break;
        }
        case NickIgnoreDomain:
        {
            IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
            if (!ircUser)
                break;
            Client::ignoreListManager()->requestAddIgnoreListItem(IgnoreListManager::SenderIgnore,
                action->property("ignoreRule").toString(),
                false, IgnoreListManager::SoftStrictness,
                IgnoreListManager::NetworkScope,
                ircUser->network()->networkName(), true);
            break;
        }
        case NickIgnoreCustom:
            // forward that to mainwin since we can access the settingspage only from there
            emit showIgnoreList(action->property("ignoreRule").toString());
            break;
        case NickIgnoreToggleEnabled0:
        case NickIgnoreToggleEnabled1:
        case NickIgnoreToggleEnabled2:
        case NickIgnoreToggleEnabled3:
        case NickIgnoreToggleEnabled4:
            Client::ignoreListManager()->requestToggleIgnoreRule(action->property("ignoreRule").toString());
            break;
        default:
            qWarning() << "Unhandled nick action";
        }
    }
}


/***************************************************************************************************************
 * JoinDlg
 ***************************************************************************************************************/

NetworkModelController::JoinDlg::JoinDlg(const QModelIndex &index, QWidget *parent) : QDialog(parent)
{
    setWindowIcon(QIcon::fromTheme("irc-join-channel"));
    setWindowTitle(tr("Join Channel"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("Network:")), 0, 0);
    layout->addWidget(networks = new QComboBox, 0, 1);
    layout->addWidget(new QLabel(tr("Channel:")), 1, 0);
    layout->addWidget(channel = new QLineEdit, 1, 1);
    layout->addWidget(new QLabel(tr("Password:")), 2, 0);
    layout->addWidget(password = new QLineEdit, 2, 1);
    layout->addWidget(buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel), 3, 0, 1, 2);
    setLayout(layout);

    channel->setFocus();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    networks->setInsertPolicy(QComboBox::InsertAlphabetically);
    password->setEchoMode(QLineEdit::Password);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(channel, SIGNAL(textChanged(QString)), SLOT(on_channel_textChanged(QString)));

    foreach(NetworkId id, Client::networkIds()) {
        const Network *net = Client::network(id);
        if (net->isConnected()) {
            networks->addItem(net->networkName(), QVariant::fromValue<NetworkId>(id));
        }
    }

    if (index.isValid()) {
        NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
        if (networkId.isValid()) {
            networks->setCurrentIndex(networks->findText(Client::network(networkId)->networkName()));
            if (index.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer
                && !index.data(NetworkModel::ItemActiveRole).toBool())
                channel->setText(index.data(Qt::DisplayRole).toString());
        }
    }
}


NetworkId NetworkModelController::JoinDlg::networkId() const
{
    return networks->itemData(networks->currentIndex()).value<NetworkId>();
}


QString NetworkModelController::JoinDlg::channelName() const
{
    return channel->text();
}


QString NetworkModelController::JoinDlg::channelPassword() const
{
    return password->text();
}


void NetworkModelController::JoinDlg::on_channel_textChanged(const QString &text)
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
}

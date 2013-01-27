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

#include "clientuserinputhandler.h"

#include "buffermodel.h"
#include "client.h"
#include "clientaliasmanager.h"
#include "clientsettings.h"
#include "execwrapper.h"
#include "ircuser.h"
#include "network.h"
#include "types.h"
#include "bufferinfo.h"
#include "clientbufferviewconfig.h"
#include "clientbufferviewmanager.h"
#include "messagemodel.h"

#include <QDateTime>

ClientUserInputHandler::ClientUserInputHandler(QObject *parent)
    : BasicHandler(parent)
{
    TabCompletionSettings s;
    s.notify("CompletionSuffix", this, SLOT(completionSuffixChanged(QVariant)));
    completionSuffixChanged(s.completionSuffix());
}


void ClientUserInputHandler::completionSuffixChanged(const QVariant &v)
{
    QString suffix = v.toString();
    QString letter = "A-Za-z";
    QString special = "\x5b-\x60\x7b-\x7d";
    _nickRx = QRegExp(QString("^([%1%2][%1%2\\d-]*)%3").arg(letter, special, suffix).trimmed());
}


// this would be the place for a client-side hook
void ClientUserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg)
{
    if (msg.isEmpty())
        return;

    if (!msg.startsWith('/')) {
        if (_nickRx.indexIn(msg) == 0) {
            const Network *net = Client::network(bufferInfo.networkId());
            IrcUser *user = net ? net->ircUser(_nickRx.cap(1)) : 0;
            if (user)
                user->setLastSpokenTo(bufferInfo.bufferId(), QDateTime::currentDateTime().toUTC());
        }
    }

    AliasManager::CommandList clist = Client::aliasManager()->processInput(bufferInfo, msg);

    for (int i = 0; i < clist.count(); i++) {
        QString cmd = clist.at(i).second.section(' ', 0, 0).remove(0, 1).toUpper();
        QString payload = clist.at(i).second.section(' ', 1);
        handle(cmd, Q_ARG(BufferInfo, clist.at(i).first), Q_ARG(QString, payload));
    }
}


void ClientUserInputHandler::defaultHandler(const QString &cmd, const BufferInfo &bufferInfo, const QString &text)
{
    QString command = QString("/%1 %2").arg(cmd, text);
    emit sendInput(bufferInfo, command);
}


void ClientUserInputHandler::handleExec(const BufferInfo &bufferInfo, const QString &execString)
{
    ExecWrapper *exec = new ExecWrapper(this); // gets suicidal when it's done
    exec->start(bufferInfo, execString);
}


void ClientUserInputHandler::handleJoin(const BufferInfo &bufferInfo, const QString &text)
{
    if (text.isEmpty()) {
        Client::messageModel()->insertErrorMessage(bufferInfo, tr("/JOIN expects a channel"));
        return;
    }
    switchBuffer(bufferInfo.networkId(), text.section(' ', 0, 0));
    // send to core
    defaultHandler("JOIN", bufferInfo, text);
}


void ClientUserInputHandler::handleQuery(const BufferInfo &bufferInfo, const QString &text)
{
    if (text.isEmpty()) {
        Client::messageModel()->insertErrorMessage(bufferInfo, tr("/QUERY expects at least a nick"));
        return;
    }
    switchBuffer(bufferInfo.networkId(), text.section(' ', 0, 0));
    // send to core
    defaultHandler("QUERY", bufferInfo, text);
}


void ClientUserInputHandler::switchBuffer(const NetworkId &networkId, const QString &bufferName)
{
    BufferId newBufId = Client::networkModel()->bufferId(networkId, bufferName);
    if (!newBufId.isValid()) {
        Client::bufferModel()->switchToBufferAfterCreation(networkId, bufferName);
    }
    else {
        Client::bufferModel()->switchToBuffer(newBufId);
        // unhide the buffer
        ClientBufferViewManager *clientBufferViewManager = Client::bufferViewManager();
        QList<ClientBufferViewConfig *> bufferViewConfigList = clientBufferViewManager->clientBufferViewConfigs();
        foreach(ClientBufferViewConfig *bufferViewConfig, bufferViewConfigList) {
            if (bufferViewConfig->temporarilyRemovedBuffers().contains(newBufId)) {
                bufferViewConfig->addBuffer(newBufId, bufferViewConfig->bufferList().length());
                //if (bufferViewConfig->sortAlphabetically()) {
                // TODO we need to trigger a sort here, but can't reach the model required
                // to get a bufferviewfilter, as the bufferviewmanager only managers configs
                //BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
                //}
            }
        }
    }
}

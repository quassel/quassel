/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "metricsserver.h"

#include <utility>

#include <QByteArray>
#include <QDebug>
#include <QHostAddress>
#include <QStringList>
#include <QTcpSocket>

#include "core.h"
#include "corenetwork.h"

MetricsServer::MetricsServer(QObject* parent)
    : QObject(parent)
{
    connect(&_server, &QTcpServer::newConnection, this, &MetricsServer::incomingConnection);
    connect(&_v6server, &QTcpServer::newConnection, this, &MetricsServer::incomingConnection);
}

bool MetricsServer::startListening()
{
    bool success = false;

    uint16_t port = Quassel::optionValue("metrics-port").toUShort();

    const QString listen = Quassel::optionValue("metrics-listen");
    const QStringList listen_list = listen.split(",", Qt::SkipEmptyParts);
    for (const QString& listen_term : listen_list) { // TODO: handle multiple interfaces for same TCP version gracefully
        QHostAddress addr;
        if (!addr.setAddress(listen_term)) {
            qCritical() << qPrintable(
                tr("Invalid listen address %1")
                    .arg(listen_term)
            );
        }
        else {
            switch (addr.protocol()) {
            case QAbstractSocket::IPv6Protocol:
                if (_v6server.listen(addr, port)) {
                    qInfo() << qPrintable(
                        tr("Listening for metrics requests on IPv6 %1 port %2")
                            .arg(addr.toString())
                            .arg(_v6server.serverPort())
                    );
                    success = true;
                }
                else
                    qWarning() << qPrintable(
                        tr("Could not open IPv6 interface %1:%2: %3")
                            .arg(addr.toString())
                            .arg(port)
                            .arg(_v6server.errorString()));
                break;
            case QAbstractSocket::IPv4Protocol:
                if (_server.listen(addr, port)) {
                    qInfo() << qPrintable(
                        tr("Listening for metrics requests on IPv4 %1 port %2")
                            .arg(addr.toString())
                            .arg(_server.serverPort())
                    );
                    success = true;
                }
                else {
                    // if v6 succeeded on Any, the port will be already in use - don't display the error then
                    if (!success || _server.serverError() != QAbstractSocket::AddressInUseError)
                        qWarning() << qPrintable(
                            tr("Could not open IPv4 interface %1:%2: %3")
                                .arg(addr.toString())
                                .arg(port)
                                .arg(_server.errorString()));
                }
                break;
            default:
                qCritical() << qPrintable(
                    tr("Invalid listen address %1, unknown network protocol")
                        .arg(listen_term)
                );
                break;
            }
        }
    }

    if (!success) {
        qWarning() << qPrintable(tr("Metrics could not open any network interfaces to listen on! No metrics functionality will be available"));
    }

    return success;
}

void MetricsServer::stopListening(const QString& msg)
{
    bool wasListening = false;

    if (_server.isListening()) {
        wasListening = true;
        _server.close();
    }
    if (_v6server.isListening()) {
        wasListening = true;
        _v6server.close();
    }

    if (wasListening) {
        if (msg.isEmpty())
            qInfo() << "No longer listening for metrics requests.";
        else
            qInfo() << qPrintable(msg);
    }
}

void MetricsServer::incomingConnection()
{
    auto server = qobject_cast<QTcpServer*>(sender());
    Q_ASSERT(server);
    while (server->hasPendingConnections()) {
        QTcpSocket* socket = server->nextPendingConnection();
        connect(socket, &QIODevice::readyRead, this, &MetricsServer::respond);
        connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);
    }
}

QString parseHttpString(const QByteArray& request, int& index)
{
    QString content;
    int end = request.indexOf(' ', index);
    if (end == -1) {
        end = request.length();
    }

    if (end > -1) {
        content = QString::fromUtf8(request.mid(index, end - index));
        index = end + 1;
    }
    else {
        index = request.length();
    }
    return content;
}

void MetricsServer::respond()
{
    auto socket = qobject_cast<QTcpSocket*>(sender());
    Q_ASSERT(socket);

    if (!socket->canReadLine()) {
        return;
    }

    int index = 0;
    QString verb;
    QString requestPath;
    QString version;
    QByteArray request;
    for (int i = 0; i < 5 && verb == ""; i++) {
        request = socket->readLine(4096);
        if (request.endsWith("\r\n")) {
            request.chop(2);
        }
        else if (request.endsWith("\n")) {
            request.chop(1);
        }

        verb = parseHttpString(request, index);
        requestPath = parseHttpString(request, index);
        version = parseHttpString(request, index);
    }

    if (requestPath == "/metrics") {
        if (version == "HTTP/1.1") {
            socket->write(
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain; version=0.0.4\r\n"
                "Connection: close\r\n"
                "\r\n"
            );
        }
        int64_t timestamp = QDateTime::currentMSecsSinceEpoch();
        for (const auto& key : _sessions.keys()) {
            const QString& name = _sessions[key];
            socket->write("# HELP quassel_network_bytes_received Number of currently open connections from quassel clients\n");
            socket->write("# TYPE quassel_client_sessions gauge\n");
            socket->write(
                QString("quassel_client_sessions{user=\"%1\"} %2 %3\n")
                    .arg(name)
                    .arg(_clientSessions.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
            socket->write("# HELP quassel_network_bytes_received Number of currently open connections to IRC networks\n");
            socket->write("# TYPE quassel_network_sessions gauge\n");
            socket->write(
                QString("quassel_network_sessions{user=\"%1\"} %2 %3\n")
                    .arg(name)
                    .arg(_networkSessions.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
            socket->write("# HELP quassel_network_bytes_received Amount of bytes sent to IRC\n");
            socket->write("# TYPE quassel_network_bytes_sent counter\n");
            socket->write(
                QString("quassel_network_bytes_sent{user=\"%1\"} %2 %3\n")
                    .arg(name)
                    .arg(_networkDataTransmit.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
            socket->write("# HELP quassel_network_bytes_received Amount of bytes received from IRC\n");
            socket->write("# TYPE quassel_network_bytes_received counter\n");
            socket->write(
                QString("quassel_network_bytes_received{user=\"%1\"} %2 %3\n")
                    .arg(name)
                    .arg(_networkDataReceive.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
            socket->write("# HELP quassel_message_queue The number of messages currently queued for that user\n");
            socket->write("# TYPE quassel_message_queue gauge\n");
            socket->write(
                QString("quassel_message_queue{user=\"%1\"} %2 %3\n")
                    .arg(name)
                    .arg(_messageQueue.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
            socket->write("# HELP quassel_login_attempts The number of times the user has attempted to log in\n");
            socket->write("# TYPE quassel_login_attempts counter\n");
            socket->write(
                QString("quassel_login_attempts{user=\"%1\",successful=\"false\"} %2 %3\n")
                    .arg(name)
                    .arg(_loginAttempts.value(key, 0) - _successfulLogins.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
            socket->write(
                QString("quassel_login_attempts{user=\"%1\",successful=\"true\"} %2 %3\n")
                    .arg(name)
                    .arg(_successfulLogins.value(key, 0))
                    .arg(timestamp)
                    .toUtf8()
            );
        }
        if (!_certificateExpires.isNull()) {
            socket->write("# HELP quassel_ssl_expire_time_seconds Expiration of the current TLS certificate in unixtime\n");
            socket->write("# TYPE quassel_ssl_expire_time_seconds gauge\n");
            socket->write(
                QString("quassel_ssl_expire_time_seconds %1 %2\n")
                    .arg(_certificateExpires.toMSecsSinceEpoch() / 1000)
                    .arg(timestamp)
                    .toUtf8()
            );
        }
        socket->close();
    }
    else if (requestPath == "/healthz") {
        if (version == "HTTP/1.1") {
            socket->write(
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n"
                "\r\n"
            );
        }
        socket->write(
            "OK\n"
        );
        socket->close();
    }
    else {
        if (version == "HTTP/1.1") {
            socket->write(
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
            );
        }
        socket->write(
            QString(
                "<html>\n"
                "<head><title>404 Not Found</title></head>\n"
                "<body>\n"
                "<center><h1>404 Not Found</h1></center>\n"
                "<hr><center>quassel %1 </center>\n"
                "</body>\n"
                "</html>\n")
                .arg(Quassel::buildInfo().baseVersion)
                .toUtf8()
        );
        socket->close();
    }
}

void MetricsServer::addLoginAttempt(UserId user, bool successful) {
    _loginAttempts.insert(user, _loginAttempts.value(user, 0) + 1);
    if (successful) {
        _successfulLogins.insert(user, _successfulLogins.value(user, 0) + 1);
    }
}

void MetricsServer::addLoginAttempt(const QString& user, bool successful) {
    UserId userId = _sessions.key(user);
    if (userId.isValid()) {
        addLoginAttempt(userId, successful);
    }
}

void MetricsServer::addSession(UserId user, const QString& name)
{
    _sessions.insert(user, name);
}

void MetricsServer::removeSession(UserId user)
{
    _sessions.remove(user);
}

void MetricsServer::addClient(UserId user)
{
    _clientSessions.insert(user, _clientSessions.value(user, 0) + 1);
}

void MetricsServer::removeClient(UserId user)
{
    int32_t count = _clientSessions.value(user, 0) - 1;
    if (count <= 0) {
        _clientSessions.remove(user);
    }
    else {
        _clientSessions.insert(user, count);
    }
}

void MetricsServer::addNetwork(UserId user)
{
    _networkSessions.insert(user, _networkSessions.value(user, 0) + 1);
}

void MetricsServer::removeNetwork(UserId user)
{
    int32_t count = _networkSessions.value(user, 0) - 1;
    if (count <= 0) {
        _networkSessions.remove(user);
    }
    else {
        _networkSessions.insert(user, count);
    }
}

void MetricsServer::transmitDataNetwork(UserId user, uint64_t size)
{
    _networkDataTransmit.insert(user, _networkDataTransmit.value(user, 0) + size);
}

void MetricsServer::receiveDataNetwork(UserId user, uint64_t size)
{
    _networkDataReceive.insert(user, _networkDataReceive.value(user, 0) + size);
}

void MetricsServer::messageQueue(UserId user, uint64_t size)
{
    _messageQueue.insert(user, size);
}

void MetricsServer::setCertificateExpires(QDateTime expires)
{
    _certificateExpires = std::move(expires);
}

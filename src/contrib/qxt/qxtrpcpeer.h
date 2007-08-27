/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtNetwork module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE. 
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
** 
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
*****************************************************************************
**
** This file has been modified from its original state to suit the needs of
** Quassel IRC. We have virtualized some methods.
**
*****************************************************************************/

#ifndef QXTRPCPEER
#define QXTRPCPEER

#include <QObject>
#include <QList>
#include <QVariant>
#include <QPair>
#include <QString>
#include <QHostAddress>
#include <qxtpimpl.h>
#include <qxtglobal.h>

class QxtRPCPeerPrivate;
/*!
 * \class QxtRPCPeer QxtRPCPeer
 * \ingroup network
 * \brief Transmits Qt signals over a network connection
 *
 * QxtRPCPeer is a tool that encapsulates Qt signals and transmits them over a network connection.
 * The signal is subsequently re-emitted on the receiving end of the connection.
 * 
 * QxtRPCPeer can operate in peer-to-peer mode (that is, one-to-one) or client-server (that is, one-to-many) mode.
 * In peer or server mode, QxtRPCPeer can listen for and accept incoming connections. In peer or client mode,
 * QxtRPCPeer can connect to a listening peer or server.
 * 
 * All data types used in attached signals and slots must be declared and registered with QMetaType using
 * Q_DECLARE_METATYPE and qRegisterMetaType, and they must have stream operators registered with qRegisterMetaTypeStreamOperators.
 *
 * The limits on the number of parameters passed to call() and related functions are a restriction of Qt,
 * which limits parameters on a signal or slot to 10.
 */ 
class QXT_NETWORK_EXPORT QxtRPCPeer : public QObject {
Q_OBJECT
public:

    /*!
     * This enum is used with the \a setRPCType() to describe the role played in a connection. It is also returned by \a rpcType().
     */
    enum RPCTypes {
        Server, /**< Listen for clients and accept multiple connections. */
        Client, /**< Connect to a server. */
        Peer    /**< Listen for a connection or connect to a peer. */
    };

    /*!
    * Creates a QxtRPCPeer object with the given parent. Unless changed later, this object will use Peer mode and QTcpSocket for its I/O device.
    */
    QxtRPCPeer(QObject* parent = 0);

    /*!
    * Creates a QxtRPCPeer object with the given parent and type. Unless changed later, this object will use QTcpSocket for its I/O device.
    */
    QxtRPCPeer(RPCTypes type, QObject* parent = 0);

    /*!
    * Creates a QxtRPCPeer object with the given parent and type and connects it to the specified I/O device.
    * 
    * Note that the I/O device must already be opened for reading and writing. This constructor cannot be used for Server mode.
    */
    QxtRPCPeer(QIODevice* device, RPCTypes type = QxtRPCPeer::Peer, QObject* parent = 0);

    /*!
     * Sets the RPC type. 
     *
     * Attempting to change the RPC type while listening or connected will be ignored with a warning.
     */
    void setRPCType(RPCTypes type);

    /*!
     * Returns the current RPC type.
     */
    RPCTypes rpcType() const;

    /*!
     * Connects to the specified peer or server on the selected port.
     *
     * When the connection is complete, the \a peerConnected() signal will be emitted.  If an error occurs, the \a peerError() signal will be emitted.
     */
    void connect(QHostAddress addr, int port = 80);

    /*!
     * Listens on the specified interface on the specified port for connections. 
     *
     * Attempting to listen while in Client mode or while connected in Peer mode will be ignored with a warning.  In Peer mode, only one connection
     * can be active at a time. Additional incoming connections while connected to a peer will be dropped. When a peer connects, the \a peerConnected()
     * signal will be emitted. In Server mode, multiple connections can be active at a time. Each client that connects will be provided a unique ID,
     * included in the \a clientConnected() signal that will be emitted.
     */
    bool listen(QHostAddress iface = QHostAddress::Any, int port = 80);

    /*!
     * Disconnects from a server, client, or peer.
     *
     * Servers must provide a client ID, provided by the \a clientConnected() signal; clients and peers must not.
     */
    void disconnectPeer(quint64 id = -1);

    /*!
     * Disconnects from all clients, or from the server or peer.
     */
    void disconnectAll();

    /*!
     * Stops listening for connections. Any connections still open will remain connected.
     */
    void stopListening();
        
    /*!
     * Returns a list of client IDs for all connected clients.
     */
    QList<quint64> clients() const;

    /*!
     * Attaches the given signal. 
     *
     * When the attached signal is emitted, it will be transmitted to all connected servers, clients, or peers.
     * If an optional rpcFunction is provided, it will be used in place of the name of the transmitted signal.
     * Use the SIGNAL() macro to specify the signal, just as you would for QObject::connect().
     *
     * Like QObject::connect(), attachSignal returns false if the connection cannot be established.
     */
    bool attachSignal(QObject* sender, const char* signal, const QByteArray& rpcFunction = QByteArray());

    /*!
     * Attaches the given slot. 
     *
     * When a signal with the name given by rpcFunction is received from the network, the attached slot is executed. 
     * Use the SLOT() macro to specify the slot, just as you would for QObject::connect(). 
     *
     * Like QObject::connect(), attachSignal returns false if the connection cannot be established.
     *
     * \Note In Server mode, the first parameter of the slot must be int id. The parameters of the signal follow.
     * For example, SIGNAL(mySignal(QString)) from the client connects to SLOT(mySlot(int, QString)) on the server.
     */
    bool attachSlot(const QByteArray& rpcFunction, QObject* recv, const char* slot);

    /*!
     * Detaches all signals and slots for the given object.
     */
    void detachObject(QObject* obj);

public slots:
    /*!
     * Sends the signal fn with the given parameter list to the server or peer. 
     *
     * This function accepts up to 9 QVariant parameters. 
     *
     * The receiver is not obligated to act upon the signal. If no server or peer is connected, the call is ignored.
     * In particular, this function does nothing in Server mode.
     */
    void call(const char *signal, QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(), QVariant p4 = QVariant(),
              QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant(), QVariant p8 = QVariant(), QVariant p9 = QVariant());

    /*!
     * Sends the signal with the given parameter list to the provided list of clients.
     *
     * This function accepts up to 8 QVariant parameters.
     *
     * The receivers are not obligated to act upon the signal. If no client is connected with a provided ID, the ID
     * is ignored with a warning.
     */
    void callClientList(QList<quint64> ids, QString fn, QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(), QVariant p4 = QVariant(),
              QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant(), QVariant p8 = QVariant());

    /*!
     * Sends the signal fn with the given parameter list to the specified client.
     *
     * This function accepts up to 8 QVariant parameters. 
     * 
     * The receiver is not obligated to act upon the signal. If no client with the given ID is connected, the call will be ignored with a warning.
     */
    void callClient(quint64 id, QString fn, QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(), QVariant p4 = QVariant(),
              QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant(), QVariant p8 = QVariant());

    /*!
     * Sends the signal fn with the given parameter list to all connected clients except for the client specified.
     *
     * This function accepts up to 8 QVariant parameters.
     *
     * The receiver is not obligated to act upon the signal. This function is useful for rebroadcasting a signal from one client
     * to all other connected clients.
     */
    void callClientsExcept(quint64 id, QString fn, QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(), QVariant p4 = QVariant(),
              QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant(), QVariant p8 = QVariant());

    /*!
     * Detaches all signals and slots for the object that emitted the signal connected to detachSender().
     */
    void detachSender();

signals:
    /*!
     * This signal is emitted after a successful connection to or from a peer or server.
     */
    void peerConnected();

    /*!
     * This signal is emitted after a successful connection from a client. 
     *
     * The given ID is used for disconnectPeer(), callClient(), and related functions.
     */
    void clientConnected(quint64 id);

    /*!
     * This signal is emitted when a peer or server is disconnected.
     */
    void peerDisconnected();

    /*!
     * This signal is emitted when a client disconnects. The given ID is no longer valid.
     */
    void clientDisconnected(quint64 id);

    /*!
     * This signal is emitted whenever an error occurs on a socket.
     *
     * Currently, no information about the socket that raised the error is available.
     */
    void peerError(QAbstractSocket::SocketError);

protected:
    /*!
     * Serializes a signal into a form suitable for transmitting over the network.
     *
     * Reimplement this function in a subclass to allow QxtRPCPeer to use a different protocol.
     */
    virtual QByteArray serialize(QString fn, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8, QVariant p9) const;

    /*!
     * Deserializes network data into a signal name and a list of parameters.
     *
     * Reimplement this function in a subclass to allow QxtRPCPeer to understand a different protocol.
     * If you reimplement it, be sure to remove the processed portion of the data from the reference parameter.
     * Return "qMakePair(QString(), QList<QVariant>())" if the deserialized data doesn't invoke a signal.
     * Return "qMakePair(QString(), QList<QVariant>() << QVariant())" if the protocol has been violated and
     * the connection should be severed.
     */
    virtual QPair<QString, QList<QVariant> > deserialize(QByteArray& data);

    /*!
     * Indicates whether the data currently received from the network can be deserialized.
     *
     * The default behavior of this function returns true if the buffer contains a newline character.
     *
     * Reimplement this function in a subclass to allow QxtRPCPeer to understand a different protocol.
     */
    virtual bool canDeserialize(const QByteArray& buffer) const;

//protected:
//    void newConnection();

private:
    QXT_DECLARE_PRIVATE(QxtRPCPeer);

private slots:
    void newConnection();
    void dataAvailable();
    void disconnectSender();
};
#endif

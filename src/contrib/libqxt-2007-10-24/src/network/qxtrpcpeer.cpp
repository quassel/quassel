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
 ****************************************************************************/

#include "qxtrpcpeer.h"
#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QMultiHash>
#include <QDebug>
#include <QMetaMethod>
#include <cassert>
#include "qxtmetaobject.h"
#include <QStack>

class QxtIntrospector: public QObject
{
// This class MANUALLY implements the necessary parts of QObject.
// Do NOT add the Q_OBJECT macro. As this class isn't intended
// for direct use, it doesn't offer any sort of useful meta-object.
public:
    QxtIntrospector(QxtRPCPeer* parent, QObject* source, const char* signal);

    int qt_metacall(QMetaObject::Call _c, int _id, void **_a);

    QString rpcFunction;

private:
    QxtRPCPeer* peer;
    QList<int> argTypes;
};

struct QxtRPCConnection
{
    QTcpSocket* socket;
    QByteArray buffer;
    QString lastMethod;
};

class QxtRPCPeerPrivate : public QxtPrivate<QxtRPCPeer>, public QTcpServer
{
public:
    QXT_DECLARE_PUBLIC(QxtRPCPeer);

    void incomingConnection ( int socketDescriptor );


    void receivePeerSignal(QString fn, QVariant p0 = QVariant(), QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(),
                           QVariant p4 = QVariant(), QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant(), QVariant p8 = QVariant()) const;
    void receiveClientSignal(quint64 id, QString fn, QVariant p0 = QVariant(), QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(),
                             QVariant p4 = QVariant(), QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant()) const;

    void processInput(QIODevice* socket, QByteArray& buffer);

    // Object -> introspector for each signal
    QMultiHash<QObject*, QxtIntrospector*> attachedSignals;
    // RPC function -> (object, slot ID)
    typedef QPair<QObject*, int> MethodID;
    QHash<QString, QList<MethodID> > attachedSlots;

    typedef QHash<QObject*, QxtRPCConnection*> ConnHash;
    ConnHash m_clients;
    QIODevice* m_peer;

    QByteArray m_buffer;
    int m_rpctype;


    QStack<QTcpSocket*> pending_connections;

};

QxtRPCPeer::QxtRPCPeer(QObject* parent) : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtRPCPeer);
    qxt_d().m_rpctype = Peer;
    qxt_d().m_peer = new QTcpSocket(this);
    QObject::connect(qxt_d().m_peer, SIGNAL(connected()), this, SIGNAL(peerConnected()));
    QObject::connect(qxt_d().m_peer, SIGNAL(disconnected()), this, SIGNAL(peerDisconnected()));
    QObject::connect(qxt_d().m_peer, SIGNAL(disconnected()), this, SLOT(disconnectSender()));
    QObject::connect(qxt_d().m_peer, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    QObject::connect(qxt_d().m_peer, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(peerError(QAbstractSocket::SocketError)));
}


QxtRPCPeer::QxtRPCPeer(RPCTypes type, QObject* parent) : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtRPCPeer);
    qxt_d().m_rpctype = type;
    qxt_d().m_peer = new QTcpSocket(this);
    QObject::connect(qxt_d().m_peer, SIGNAL(connected()), this, SIGNAL(peerConnected()));
    QObject::connect(qxt_d().m_peer, SIGNAL(disconnected()), this, SIGNAL(peerDisconnected()));
    QObject::connect(qxt_d().m_peer, SIGNAL(disconnected()), this, SLOT(disconnectSender()));
    QObject::connect(qxt_d().m_peer, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    QObject::connect(qxt_d().m_peer, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(peerError(QAbstractSocket::SocketError)));
}


QxtRPCPeer::QxtRPCPeer(QIODevice* device, RPCTypes type, QObject* parent) : QObject(parent)
{
    if (!device->isOpen())
    {
        qWarning("QxtRPCPeer::the device you passed is not open!");
    }

    QXT_INIT_PRIVATE(QxtRPCPeer);
    qxt_d().m_rpctype = type;
    qxt_d().m_peer = device;

    if (qobject_cast<QAbstractSocket *>(device)!=0)
    {
        QObject::connect(qxt_d().m_peer, SIGNAL(connected()), this, SIGNAL(peerConnected()));
        QObject::connect(qxt_d().m_peer, SIGNAL(disconnected()), this, SIGNAL(peerDisconnected()));
        QObject::connect(qxt_d().m_peer, SIGNAL(disconnected()), this, SLOT(disconnectSender()));
        QObject::connect(qxt_d().m_peer, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(peerError(QAbstractSocket::SocketError)));
    }
    QObject::connect(qxt_d().m_peer, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
}


void QxtRPCPeer::setRPCType(RPCTypes type)
{
    if (qxt_d().m_peer->isOpen () || qxt_d().isListening())
    {
        qWarning() << "QxtRPCPeer: Cannot change RPC types while connected or listening";
        return;
    }
    qxt_d().m_rpctype = type;
}


QxtRPCPeer::RPCTypes QxtRPCPeer::rpcType() const
{
    return (RPCTypes)(qxt_d().m_rpctype);
}


void QxtRPCPeer::connect(QHostAddress addr, int port)
{
    if (qxt_d().m_rpctype == Server)
    {
        qWarning() << "QxtRPCPeer: Cannot connect outward in Server mode";
        return;
    }

    QAbstractSocket * sock  = qobject_cast<QAbstractSocket*>(qxt_d().m_peer);
    if (!sock)
    {
        qWarning("QxtRPCPeer: cannot connect a custom QIODevice");
        return;
    }

    if (sock->state()!=QAbstractSocket::UnconnectedState)
    {
        qWarning("QxtRPCPeer: Already connected");
        return;
    }

    sock->connectToHost(addr, port);
}


bool QxtRPCPeer::listen(QHostAddress iface, int port)
{
    if (qxt_d().m_rpctype == Client)
    {
        qWarning() << "QxtRPCPeer: Cannot listen in Client mode";
        return false;
    }
    else if (qxt_d().m_rpctype == Peer && qxt_d().m_peer->isOpen ())
    {
        qWarning() << "QxtRPCPeer: Cannot listen while connected to a peer";
        return false;
    }
    else if (qxt_d().isListening())
    {
        qWarning() << "QxtRPCPeer: Already listening";
        return false;
    }
    return qxt_d().listen(iface, port);
}


void QxtRPCPeer::disconnectPeer(quint64 id)
{
    if (qxt_d().m_rpctype == Server && id==(quint64)-1)
    {
        qWarning() << "QxtRPCPeer: Server mode does not have a peer";
        return;
    }
    else if (qxt_d().m_rpctype!= Server && id!=(quint64)-1)
    {
        qWarning() << "QxtRPCPeer: Must specify a client ID to disconnect";
        return;
    }
    QxtRPCConnection* conn;
    if (id==(quint64)-1)
    {
        qxt_d().m_peer->close();
        ///hackaround for qt bug
        QAbstractSocket *s =qobject_cast<QAbstractSocket*>( qxt_d().m_peer);
        if (s)
            s->disconnectFromHost();

    }
    else if ((conn = qxt_d().m_clients.take((QObject*)(id)))!= 0)
    {
        conn->socket->disconnectFromHost();
        conn->socket->deleteLater();
        delete conn;
    }
    else
    {
        qWarning() << "QxtRPCPeer: no client with id " << id;
    }
}


void QxtRPCPeer::disconnectAll()
{
    if (qxt_d().m_rpctype!= Server)
        disconnectPeer();
    else
    {
        for (QxtRPCPeerPrivate::ConnHash::const_iterator i = qxt_d().m_clients.constBegin(); i!= qxt_d().m_clients.constEnd(); i++)
        {
            (*i)->socket->deleteLater();
            delete *i;
        }
        qxt_d().m_clients.clear();
    }
}


void QxtRPCPeer::stopListening()
{
    if (!qxt_d().isListening())
    {
        qWarning() << "QxtRPCPeer: Not listening";
        return;
    }
    qxt_d().close();
}


bool QxtRPCPeer::attachSignal(QObject* sender, const char* signal, const QByteArray& rpcFunction)
{
    const QMetaObject* meta = sender->metaObject();
    QByteArray sig(meta->normalizedSignature(signal).mid(1));
    int methodID = meta->indexOfMethod(sig.constData());
    if (methodID == -1 || meta->method(methodID).methodType() != QMetaMethod::Signal)
    {
        qWarning() << "QxtRPCPeer::attachSignal: No such signal " << signal;
        return false;
    }


    QxtIntrospector* spec = new QxtIntrospector(this, sender, signal);
    if (!rpcFunction.isEmpty())
    {
        if (QxtMetaObject::isSignalOrSlot(rpcFunction.constData()))
        {
            spec->rpcFunction = QMetaObject::normalizedSignature(rpcFunction);
        }
        else
        {
            spec->rpcFunction  = rpcFunction.simplified();
        }
    }
    else
    {
        spec->rpcFunction = QMetaObject::normalizedSignature(signal);
    }
    qxt_d().attachedSignals.insertMulti(sender, spec);
    return true;
}


bool QxtRPCPeer::attachSlot(const QByteArray& rpcFunction, QObject* recv, const char* slot)
{
    const QMetaObject* meta = recv->metaObject();
    int methodID = meta->indexOfMethod(meta->normalizedSignature(slot).mid(1));
    if (methodID == -1 || meta->method(methodID).methodType() == QMetaMethod::Method)
    {
        qWarning() << "QxtRPCPeer::attachSlot: No such slot " << slot;
        return false;
    }

    QString fn;

    if (QxtMetaObject::isSignalOrSlot(rpcFunction.constData()))
    {
        fn = QMetaObject::normalizedSignature(rpcFunction.constData());
    }
    else
    {
        fn = rpcFunction.simplified();
    }

    qxt_d().attachedSlots[fn].append(QPair<QObject*, int>(recv, recv->metaObject()->indexOfMethod(recv->metaObject()->normalizedSignature(slot).mid(1))));
    return true;
}


void QxtRPCPeer::detachSender()
{
    detachObject(sender());
}


void QxtRPCPeer::detachObject(QObject* obj)
{
    foreach(QxtIntrospector* i, qxt_d().attachedSignals.values(obj)) i->deleteLater();
    qxt_d().attachedSignals.remove(obj);
    foreach(QString slot, qxt_d().attachedSlots.keys())
    {
        for (QList<QPair<QObject*, int> >::iterator i(qxt_d().attachedSlots[slot].begin());
                i!= qxt_d().attachedSlots[slot].end(); )
        {
            if ((*i).first == obj)
                i = qxt_d().attachedSlots[slot].erase(i);
            else
                i++;
        }
    }
}


QByteArray QxtRPCPeer::serialize(QString fn, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8, QVariant p9) const
{
    QByteArray rv;
    QDataStream str(&rv, QIODevice::WriteOnly);
    str << fn;
    unsigned char ct = 9;
    if (p1.isNull()) ct = 0;
    else if (p2.isNull()) ct = 1;
    else if (p3.isNull()) ct = 2;
    else if (p4.isNull()) ct = 3;
    else if (p5.isNull()) ct = 4;
    else if (p6.isNull()) ct = 5;
    else if (p7.isNull()) ct = 6;
    else if (p8.isNull()) ct = 7;
    else if (p9.isNull()) ct = 8;
    str << ct;
    if (ct-- >0 ) str << p1;
    if (ct-- >0) str << p2;
    if (ct-- >0) str << p3;
    if (ct-- >0) str << p4;
    if (ct-- >0) str << p5;
    if (ct-- >0) str << p6;
    if (ct-- >0) str << p7;
    if (ct-- >0) str << p8;
    if (ct-- >0) str << p9;
    rv.replace(QByteArray("\\"), QByteArray("\\\\"));
    rv.replace(QByteArray("\n"), QByteArray("\\n"));
    rv.append("\n");
    return rv;
}


void QxtRPCPeer::call(const char * signal , QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8, QVariant p9)
{

    QByteArray sig=QMetaObject::normalizedSignature(signal);

    QAbstractSocket * sock  = qobject_cast<QAbstractSocket*>(qxt_d().m_peer);
    if (!qxt_d().m_peer->isOpen () || ( sock && sock->state()!=QAbstractSocket::ConnectedState ))
    {
        qWarning("can't call on a closed device");
        return;
    }
    qxt_d().m_peer->write(serialize(sig, p1, p2, p3, p4, p5, p6, p7, p8, p9));
}


void QxtRPCPeer::callClientList(QList<quint64> ids, QString fn, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8)
{
    QByteArray c = serialize(fn, p1, p2, p3, p4, p5, p6, p7, p8, QVariant());
    foreach(quint64 id, ids)
    {
        QxtRPCConnection* conn = qxt_d().m_clients.value((QObject*)(id));
        if (!conn)
        {
            qWarning() << "QxtRPCPeer: no client with id" << id;
        }
        else
        {
            conn->socket->write(c);
        }
    }
}


void QxtRPCPeer::callClient(quint64 id, QString fn, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8)
{
    callClientList(QList<quint64>() << id, fn, p1, p2, p3, p4, p5, p6, p7, p8);
}


void QxtRPCPeer::callClientsExcept(quint64 id, QString fn, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8)
{
    QList<quint64> cs = clients();
    cs.removeAll(id);
    callClientList(cs, fn, p1, p2, p3, p4, p5, p6, p7, p8);
}


#define QXT_ARG(i) ((numParams>i)?QGenericArgument(p ## i .typeName(), p ## i .constData()):QGenericArgument())
void QxtRPCPeerPrivate::receivePeerSignal(QString fn, QVariant p0, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8) const
{
    QByteArray sig;
    int numParams;
    foreach(QxtRPCPeerPrivate::MethodID i, attachedSlots.value(fn))
    {
        sig = i.first->metaObject()->method(i.second).signature();
        sig = sig.left(sig.indexOf('('));
        numParams = i.first->metaObject()->method(i.second).parameterTypes().count();
        if(!QMetaObject::invokeMethod(i.first, sig, QXT_ARG(0), QXT_ARG(1), QXT_ARG(2), QXT_ARG(3), QXT_ARG(4), QXT_ARG(5), QXT_ARG(6), QXT_ARG(7), QXT_ARG(8)))
        {
            qWarning("QxtRPCPeerPrivate::receivePeerSignal: invokeMethod for \"%s\" failed ",sig.constData());
        }
    }
}


void QxtRPCPeerPrivate::receiveClientSignal(quint64 id, QString fn, QVariant p0, QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7) const
{
    QByteArray sig;
    int numParams;
    foreach(QxtRPCPeerPrivate::MethodID i, attachedSlots.value(fn))
    {
        sig = i.first->metaObject()->method(i.second).signature();
        sig = sig.left(sig.indexOf('('));
        numParams = i.first->metaObject()->method(i.second).parameterTypes().count();
        if(!QMetaObject::invokeMethod(i.first, sig, Q_ARG(quint64, id), QXT_ARG(0), QXT_ARG(1), QXT_ARG(2), QXT_ARG(3), QXT_ARG(4), QXT_ARG(5), QXT_ARG(6), QXT_ARG(7)))
        {
            qWarning("QxtRPCPeerPrivate::receiveClientSignal: invokeMethod for \"%s\" failed ",sig.constData());
        }
    }
}


#undef QXT_ARG

void QxtRPCPeerPrivate::incomingConnection ( int socketDescriptor )
{
    QTcpSocket* next = qxt_p().incomingConnection(socketDescriptor);
    if (m_rpctype == QxtRPCPeer::Peer)
    {
        if (m_peer->isOpen ())
        {
            qWarning() << "QxtRPCPeer: Rejected connection from " << next->peerAddress().toString() << "; another peer is connected";
            next->disconnectFromHost();
            next->deleteLater();
        }
        else
        {
            m_peer->deleteLater();
            m_peer = next;
            QObject::connect(m_peer, SIGNAL(connected()), &qxt_p(), SIGNAL(peerConnected()));
            QObject::connect(m_peer, SIGNAL(disconnected()), &qxt_p(), SIGNAL(peerDisconnected()));
            QObject::connect(m_peer, SIGNAL(disconnected()), &qxt_p(), SLOT(disconnectSender()));
            QObject::connect(m_peer, SIGNAL(readyRead()), &qxt_p(), SLOT(dataAvailable()));
            QObject::connect(m_peer, SIGNAL(error(QAbstractSocket::SocketError)), &qxt_p(), SIGNAL(peerError(QAbstractSocket::SocketError)));
            emit qxt_p().peerConnected();
        }
    }
    else
    {
        QxtRPCConnection* conn = new QxtRPCConnection;
        conn->socket = next;
        m_clients[next] = conn;
        QObject::connect(next, SIGNAL(disconnected()), &qxt_p(), SLOT(disconnectSender()));
        QObject::connect(next, SIGNAL(readyRead()), &qxt_p(), SLOT(dataAvailable()));
        QObject::connect(next, SIGNAL(error(QAbstractSocket::SocketError)), &qxt_p(), SIGNAL(peerError(QAbstractSocket::SocketError)));
        emit qxt_p().clientConnected((quint64)(next));
    }
}


void QxtRPCPeer::dataAvailable()
{
    if (qxt_d().m_rpctype!=QxtRPCPeer::Server && qxt_d().m_peer==sender())
    {
        qxt_d().m_buffer.append(qxt_d().m_peer->readAll());
        qxt_d().processInput(qxt_d().m_peer, qxt_d().m_buffer);
        return;
    }
    else
    {
        QxtRPCConnection* conn = qxt_d().m_clients.value(sender());
        if (!conn)
        {
            qWarning() << "QxtRPCPeer: Unrecognized client object connected to dataAvailable";
            return;
        }
        conn->buffer.append(conn->socket->readAll());
        qxt_d().processInput(conn->socket, (conn->buffer));
        return;
    }
    qWarning() << "QxtRPCPeer: Unrecognized peer object connected to dataAvailable";
}


void QxtRPCPeer::disconnectSender()
{
    QxtRPCConnection* conn = qxt_d().m_clients.value(sender());
    if (!conn)
    {
        if (qxt_d().m_peer!= qobject_cast<QIODevice*>(sender()))
        {
            qWarning() << "QxtRPCPeer: Unrecognized object connected to disconnectSender";
            return;
        }
        qxt_d().m_buffer.append(qxt_d().m_peer->readAll());
        qxt_d().m_buffer.append("\n");
        qxt_d().processInput(qxt_d().m_peer, qxt_d().m_buffer);
        qxt_d().m_buffer.clear();
        emit clientDisconnected((quint64)(sender()));
        return;
    }
    conn->buffer.append(conn->socket->readAll());
    conn->buffer.append("\n");
    qxt_d().processInput(conn->socket, conn->buffer);
    conn->socket->deleteLater();
    delete conn;
    qxt_d().m_clients.remove(sender());
}


void QxtRPCPeerPrivate::processInput(QIODevice* socket, QByteArray& buffer)
{
    while (qxt_p().canDeserialize(buffer))
    {
        QPair<QString, QList<QVariant> > sig = qxt_p().deserialize(buffer);
        if (sig.first.isEmpty())
        {
            if (sig.second.count())
            {
                qWarning() << "QxtRPCPeer: Invalid data received; disconnecting";
                if (socket == m_peer)
                    qxt_p().disconnectPeer();
                else
                    qxt_p().disconnectPeer((quint64)(socket));
                return;
            }
            continue;
        }
        while (sig.second.count() < 9) sig.second << QVariant();
        if (socket == m_peer)
        {
            receivePeerSignal(sig.first, sig.second[0], sig.second[1], sig.second[2], sig.second[3], sig.second[4], sig.second[5], sig.second[6], sig.second[7], sig.second[8]);
        }
        else
        {
            receiveClientSignal((quint64)(socket), sig.first, sig.second[0], sig.second[1], sig.second[2], sig.second[3], sig.second[4], sig.second[5], sig.second[6], sig.second[7]);
        }
    }
}


QList<quint64> QxtRPCPeer::clients() const
{
    QList<quint64> rv;
    QList<QObject*> cs = qxt_d().m_clients.keys();
    foreach(QObject* id, cs) rv << (const quint64)(id);
    return rv;
}


QxtIntrospector::QxtIntrospector(QxtRPCPeer* parent, QObject* source, const char* signal): QObject(parent)
{
    peer = parent;
    QByteArray sig_ba = QMetaObject::normalizedSignature(QByteArray(signal).mid(1));
    const char * sig=sig_ba.constData();
    int idx = source->metaObject()->indexOfSignal(sig);
    if (idx<0)
        qWarning("no such signal: %s",sig_ba.constData());

// Our "method" will have the first ID not used by the superclass.
    QMetaObject::connect(source, idx, this, QObject::staticMetaObject.methodCount());
    QObject::connect(source, SIGNAL(destroyed()), peer, SLOT(detachSender()));
    QList<QByteArray> p = source->metaObject()->method(idx).parameterTypes();
    int ct = p.count();
    for (int i=0; i<ct; i++) argTypes.append(QMetaType::type(p.value(i).constData()));
}


int QxtIntrospector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod)
    {
        if (_id==0)
        {
            QVariant v[9];
            int n = argTypes.size();
            for (int i=0; i<n; i++) v[i] = QVariant(argTypes[i], _a[i+1]);
            peer->call(rpcFunction.toUtf8().constData(), v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
        }
        _id -= 1;
    }
    return _id;
}


QPair<QString, QList<QVariant> > QxtRPCPeer::deserialize(QByteArray& data)
{
    QByteArray cmd;
    int pos = data.indexOf('\n');
    cmd = data.left(pos-1);
    data = data.mid(pos+1);
    if (cmd.length()==0) return qMakePair(QString(), QList<QVariant>());
    cmd.replace(QByteArray("\\n"), QByteArray("\n"));
    cmd.replace(QByteArray("\\\\"), QByteArray("\\"));
    QDataStream str(cmd);
    QString signal;
    unsigned char argCount;
    QList<QVariant> v;
    QVariant t;
    str >> signal >> argCount;

    if (str.status() == QDataStream::ReadCorruptData)
    {
        v << QVariant();
        return qMakePair(QString(), v);
    }

    for (int i=0; i<argCount; i++)
    {
        str >> t;
        v << t;
    }
    return qMakePair(signal, v);
}


bool QxtRPCPeer::canDeserialize(const QByteArray& buffer) const
{
    if (buffer.indexOf('\n') == -1)
    {
        return false;
    }
    return true;

}




QIODevice * QxtRPCPeer::socket()
{
    if (qxt_d().m_rpctype == Server)return 0;
    return qxt_d().m_peer;
}




QTcpSocket * QxtRPCPeer::incomingConnection ( int socketDescriptor )
{
    QTcpSocket * t = new QTcpSocket;
    t->setSocketDescriptor (socketDescriptor);
    return t;
}




const QTcpSocket * QxtRPCPeer::clientSocket(quint64 id) const
    {
    if (qxt_d().m_rpctype != Server)
        return 0;

    return  qxt_d().m_clients[(QTcpSocket*)(id)]->socket;
    }
QList<quint64> QxtRPCPeer::clients()
    {
    QList<quint64> list;
    foreach(QObject * o,qxt_d().m_clients.keys ())
        {
        list.append((quint64)o);
        }
    return list;
    }




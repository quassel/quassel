/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qt eXTension library
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

#include "qxtpipe.h"
#include <QList>
#include <QQueue>
#include <QMutableListIterator>

/**
 * \class  QxtPipe QxtPipe
 * \ingroup QxtCore
 * \brief a pipeable QIODevice
 *
 * pipes can be connected to other pipes, to exchange data \n
 * The default implementation uses a buffer. \n
 * Reimplement to make your custom class able to be connected into a pipe chain. \n
 *
 * Example usage:
 * \code
 * QxtPipe p1;
 * QxtPipe p2;
 * p1|p2;
 * p1.write("hello world");
 * qDebug()<<p2.readAll();
 * \endcode
*/


struct Connection
{
    QxtPipe * pipe;
    QIODevice::OpenMode mode;
    Qt::ConnectionType connectionType;
};

class QxtPipePrivate:public QxtPrivate<QxtPipe>
{
    public:
        QQueue<char> q;
        QList<Connection> connections;
};

/**
 * Contructs a new QxtPipe.
 */
QxtPipe::QxtPipe(QObject * parent):QIODevice(parent)
{
    setOpenMode (QIODevice::ReadWrite);
}


/** reimplemented from QIODevice*/
bool QxtPipe::isSequential () const
{
    return true;
}

/** reimplemented from QIODevice*/
qint64 QxtPipe::bytesAvailable () const
{
    return qxt_d().q.count();
}

/**
 * pipes the output of this instance to the \p other  QxtPipe using the given mode and connectiontype \n
 * connection pipes with this function can be considered thread safe \n
 *
 * Example usage:
 * \code
    QxtPipe p1;
    QxtPipe p2;
    p1.connect(&p2,QIODevice::ReadOnly);

    ///this data will go nowhere. p2 is connected to p1, but not p2 to p1.
    p1.write("hello");

    ///while this data will end up in p1
    p2.write("world");

    qDebug()<<p1.readAll();

 * \endcode

 */
bool  QxtPipe::connect   (QxtPipe * other ,QIODevice::OpenMode mode,Qt::ConnectionType connectionType)
{

    ///tell the other pipe to write into this
    if(mode & QIODevice::ReadOnly)
    {
        other->connect(this,QIODevice::WriteOnly,connectionType);
    }


    Connection c;
    c.pipe=other;
    c.mode=mode;
    c.connectionType=connectionType;
    qxt_d().connections.append(c);

    return true;
}

/**
 * cuts the pipe to the \p other QxtPipe
 */
bool QxtPipe::disconnect (QxtPipe * other )
{
    bool e=false;

    QMutableListIterator<Connection> i(qxt_d().connections);
    while (i.hasNext())
    {
        i.next();
        if(i.value().pipe==other)
        {
            i.remove();
            e=true;
            other->disconnect(this);
        }
    }

    return e;
}

/**
 * convinence function for QxtPipe::connect.
 * pipes the output of this instance to the \p other  QxtPipe in readwrite mode with autoconnection
 */
QxtPipe & QxtPipe::operator | ( QxtPipe & target)
{
    connect(&target);
    return *this;
}

/** reimplemented from QIODevice*/
qint64 QxtPipe::readData ( char * data, qint64 maxSize )
{
    QQueue<char> * q=&qxt_d().q;

    qint64 i=0;
    for (;i<maxSize;i++)
    {
        if (q->isEmpty())
            break;
        (*data++)=q->dequeue();
    }
    return i;
}

/** reimplemented from QIODevice*/
qint64 QxtPipe::writeData ( const char * data, qint64 maxSize )
{
    foreach(Connection c,qxt_d().connections)
    {

        if(!(c.mode & QIODevice::WriteOnly))
            continue;

        //we want thread safety, so we use a QByteArray instead of the raw data. that migth be slow
        QMetaObject::invokeMethod(c.pipe, "receiveData",c.connectionType,
            Q_ARG(QByteArray, data),Q_ARG(QxtPipe *,this));
            
    }
    return maxSize;
}


/** 
receiveData is called from any connected pipe to input data into this instance.
*/
qint64 QxtPipe::receiveData (QByteArray datab ,QxtPipe * sender)
{
    QQueue<char> * q=&qxt_d().q;

    const char * data =datab.constData();
    qint64 maxSize =datab.size();

    qint64 i=0;
    for (;i<maxSize;i++)
        q->enqueue(*data++);


    foreach(Connection c,qxt_d().connections)
    {

        //don't write back to sender
        if(c.pipe==sender)
             continue;

        if(!(c.mode & QIODevice::WriteOnly))
            continue;


        QMetaObject::invokeMethod(c.pipe, "receiveData",c.connectionType,
            Q_ARG(QByteArray, datab),Q_ARG(QxtPipe *,this));
    }


    if (i>0)
        emit(readyRead ());

    return maxSize;
}


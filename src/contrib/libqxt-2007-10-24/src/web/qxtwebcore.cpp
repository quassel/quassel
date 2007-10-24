/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtWeb  module of the Qt eXTension library
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
** <http://libqxt.org>  <foundation@libqxt.org>
**
****************************************************************************/
#include "qxtwebcore.h"
#include "qxtabstractwebconnector.h"
#include "qxtwebcore_p.h"

#include <QTimer>
#include <QUrl>
#include "qxtwebcontroller.h"
#include <QCoreApplication>
#include <QTcpSocket>
#include <QVariant>
#include <QtDebug>
#include <QUrl>
#include <ctime>

/*!
        \class QxtWebCore QxtWebCore
        \ingroup QxtWeb
        \brief qxtweb application core class. communicates, delegates, does all of the magic ;)


        QxtWebCore is the base class of your web application.
        it listens to the scgi protocoll

        construct one webcore object in the main function of your application.
        you must contruct it AFTER QCoreApplication and BEFORe any controllers.

        \code
        int main(int argc,char ** argv)
                {
                QCoreApplication  app(argc,argv);
                QxtWebCore core();
                core.listen(8080);
                QxtWebController controller("root");
                app.exec();
                }
        \endcode
*/

/*!
        \fn static QxtWebCore* instance();
        singleton accessor
        \fn static void send(QByteArray);
        Send data to the client. Use this rarely, but use it always when sending binary data such as images. \n
        normal text/html comunication should be done using the controllers echo() function \n
        note that after you called send the first time you cannot modify the header anymore \n
        sending may be ignored by the transport when there is no client currently handled
        \fn static QIODevice * socket();
        direct access to a iodevice for writing binary data. \n
        You shouldn't use that unless it's absolutly nessesary
        \fn static QxtError parseString(QByteArray str, post_t & POST);
        much like phps parse_string \n
        \fn static QByteArray readContent(int maxsize=5000);
        reads the content from the current socket if any has sent. \n
        returns an empty QByteArray on any error.  \n
        the content is cut at maxsize and not read from the socket.  \n
        FIXME:\warning: this function is BLOCKING.  while content is read from the client, no other requests can be handled.
        FIXME:\warning: due to paranoid timeouts this might not work for slow clients
 */


static QxtWebCore * singleton_m=0;

//-----------------------interface----------------------------
QxtWebCore::QxtWebCore(QxtAbstractWebConnector * pt):QObject()
{
    if (singleton_m)
        qFatal("you're trying to construct QxtWebCore twice!");
    qRegisterMetaType<server_t>("server_t");
    qRegisterMetaTypeStreamOperators<server_t>("server_t");

    singleton_m=this;
    QXT_INIT_PRIVATE(QxtWebCore);
    qxt_d().connector=pt;
    connect(pt,SIGNAL(aboutToClose()),this,SIGNAL(aboutToClose()));
    connect(pt,SIGNAL(incomming(server_t)),&qxt_d(),SLOT(incomming(server_t)));
}

QxtWebCore::~QxtWebCore()
{
    singleton_m=0;
}


void QxtWebCore::send(QString a)
{
    instance()->qxt_d().send(a);
}
void QxtWebCore::header(QString a,QString b)
{
    instance()->qxt_d().header(a,b);
}

server_t &  QxtWebCore::SERVER()
{
    return instance()->qxt_d().currentservert;
}

QIODevice * QxtWebCore::socket()
{
    return instance()->qxt_d().connector->socket();
}

int QxtWebCore::start (quint16 port ,const QHostAddress & address )
{
    return instance()->qxt_d().connector->start(port,address);
}

void QxtWebCore::redirect(QString location,int code)
{
    instance()->qxt_d().redirect(location,code);
}

QxtWebCore * QxtWebCore::instance()
{
    if (!singleton_m)
        qFatal("no QxtWebCore constructed");
    return singleton_m;
}
void QxtWebCore::setCodec ( QTextCodec * codec )
{
    instance()->qxt_d().decoder=codec->makeDecoder();
    instance()->qxt_d().encoder=codec->makeEncoder();
}

void QxtWebCore::close()
{
    instance()->qxt_d().close();
}
void QxtWebCore::sendHeader()
{
    instance()->qxt_d().sendheader();

}

//-----------------------implementation----------------------------




QxtWebCorePrivate::QxtWebCorePrivate(QObject *parent):QObject(parent),QxtPrivate<QxtWebCore>()
{
    connector=0;
    decoder=0;
    encoder=0;
}

void QxtWebCorePrivate::send(QString str)
{
    sendheader();

    if (encoder)
        connector->socket()->write(encoder->fromUnicode (str));
    else
        connector->socket()->write(str.toUtf8());

}
void QxtWebCorePrivate::close()
{
    sendheader();
    connector->close();
}

void QxtWebCorePrivate::sendheader()
{
    if (!header_sent)
    {
        header_sent=true;
        connector->sendHeader(answer);
    }
}
void QxtWebCorePrivate::header(QString k,QString v)
{
    if (header_sent)
        qWarning("headers already sent");
    if (encoder)
        answer[encoder->fromUnicode (k)]=encoder->fromUnicode (v);
    else
        answer[k.toUtf8()]=v.toUtf8();

}
void QxtWebCorePrivate::redirect(QString l,int code)
{
    QByteArray loc =QUrl(l).toEncoded ();

    if (loc.isEmpty())
        loc="/";
    QxtWebCore::header("Status",QString::number(code).toUtf8());
    QxtWebCore::header("Location",loc);
    send(QString("<a href=\""+loc+"\">"+loc+"</a>"));
}






void QxtWebCorePrivate::incomming(server_t  SERVER)
{
    header_sent=false;
    answer.clear();
    qDebug("%i, %s -> %s",(int)time(NULL),SERVER["HTTP_HOST"].constData(),SERVER["REQUEST_URI"].constData());


    currentservert=SERVER;

    emit(qxt_p().request());

    ///--------------find controller ------------------
    QByteArray path="404";
    QList<QByteArray> requestsplit = SERVER["REQUEST_URI"].split('/');
    if (requestsplit.count()>1)
    {
        path=requestsplit.at(1);
        if (path.trimmed().isEmpty())path="root";
    }
    else if (requestsplit.count()>0)
        path="root";

    ///--------------controller------------------

    QxtWebController * controller =qFindChild<QxtWebController *> (QCoreApplication::instance(), path );
    if (!controller)
    {
        header("Status","404");
        send("<h1>404 Controller ");
        send(path);
        send(" not found</h1>");
        close();
        qDebug("404 controller '%s' not found",path.constData());
        return;
    }

    int i=controller->invoke(SERVER);
    if (i!=0 && i!=2)
    {
        header("Status","404");
        send("<h1>");
        send(QString::number(i));
        send("</h1>Sorry,, that didn't work as expected. You might want to contact this systems administrator.");
    }
    if (i!=2) ///FIXME temporary solution for keepalive
        close();
}













//-----------------------helper----------------------------

QByteArray QxtWebCore::content(int maxsize)
{
    return instance()->qxt_d().connector->content(maxsize);
}


QxtError QxtWebCore::parseString(QByteArray content_in, post_t & POST)
{
    QList<QByteArray> posts = content_in.split('&');
    QByteArray post;
    foreach(post,posts)
    {
        QList<QByteArray> b =post.split('=');
        if (b.count()!=2)continue;
        POST[QUrl::fromPercentEncoding  ( b[0].replace("+","%20"))]=QUrl::fromPercentEncoding  ( b[1].replace("+","%20") );
    }
    QXT_DROP_OK
}






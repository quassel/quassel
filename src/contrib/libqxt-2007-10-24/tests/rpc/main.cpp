/** ***** QxtRPCPeer loopback test ******/
#include <QxtRPCPeer>
#include <qxtfifo.h>
#include <QCoreApplication>
#include <QTest>
#include <QSignalSpy>
#include <QBuffer>
#include <QDebug>
#include <QByteArray>
#include <QTcpSocket>

class RPCTest: public QObject
{
    Q_OBJECT
private:
    QxtRPCPeer* peer;
signals:
    void wave(QString);
    void counterwave(QString);
    void networkedwave(quint64,QString);


private slots:
    void initTestCase()
    {}



    void loopback()
    {
        QxtFifo io;
        QxtRPCPeer peer(&io);
        QVERIFY2(peer.attachSignal (this, SIGNAL(  wave  ( QString ) ) ),"cannot attach signal");
        QVERIFY2(peer.attachSlot (  SIGNAL(   wave (  QString  )   ),this, SIGNAL( counterwave(QString  )) ),"cannot attach slot");

        QSignalSpy spy(this, SIGNAL(counterwave(QString)));
        QSignalSpy spyr(&io, SIGNAL(readyRead()));

        emit(wave("world"));

        QCoreApplication::processEvents ();
        QCoreApplication::processEvents ();

        QVERIFY2 (spyr.count()> 0, "buffer not emitting readyRead" );

        QVERIFY2 (spy.count()> 0, "no signal received" );
        QVERIFY2 (spy.count()< 2, "wtf, two signals received?" );

        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY2(arguments.at(0).toString()=="world","argument missmatch");
    }
    void directcall()
    {
        QxtFifo io;
        QxtRPCPeer peer(&io);
        QVERIFY2(peer.attachSlot (  SIGNAL(   wave (  QString  )   ),this, SIGNAL( counterwave(QString  )) ),"cannot attach slot");

        QSignalSpy spy(this, SIGNAL(counterwave(QString)));
        QSignalSpy spyr(&io, SIGNAL(readyRead()));
        peer.call(SIGNAL(wave   ( QString   )  ),QString("world"));

        QCoreApplication::processEvents ();
        QCoreApplication::processEvents ();


        QVERIFY2 (spyr.count()> 0, "buffer not emitting readyRead" );

        QVERIFY2 (spy.count()> 0, "no signal received" );
        QVERIFY2 (spy.count()< 2, "wtf, two signals received?" );

        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY2(arguments.at(0).toString()=="world","argument missmatch");
    }

    void TcpServerIo()
    {
        QxtRPCPeer server(QxtRPCPeer::Server);
        QVERIFY2(server.attachSlot (SIGNAL(wave(QString)),this,SIGNAL(networkedwave(quint64,QString))),"cannot attach slot");


        QVERIFY(server.listen (QHostAddress::LocalHost, 23444));


        QxtRPCPeer client(QxtRPCPeer::Client);
        client.connect (QHostAddress::LocalHost, 23444);
        QVERIFY(qobject_cast<QTcpSocket*>(client.socket())->waitForConnected ( 30000 ));


        QSignalSpy spy(this, SIGNAL(networkedwave(quint64,QString)));
        client.call(SIGNAL(wave(QString)),QString("world"));


        QCoreApplication::processEvents ();
        QCoreApplication::processEvents ();
        QCoreApplication::processEvents ();

        QVERIFY2 (spy.count()> 0, "no signal received" );
        QVERIFY2 (spy.count()< 2, "wtf, two signals received?" );

        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY2(arguments.at(1).toString()=="world","argument missmatch");
    }
    void cleanupTestCase()
    {}
};

QTEST_MAIN(RPCTest)
#include "main.moc"

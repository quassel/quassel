/** ***** namedpipe loopback test ******/
#include <QxtNamedPipe>
#include <QTest>
#include <QSignalSpy>
#include <QBuffer>
#include <QDebug>
#include <QByteArray>
#include <QThread>

class QxtnamedPipeTest: public QObject
	{
	Q_OBJECT 
	private slots:
		void loopback()
			{ 
			QxtNamedPipe out("/tmp/QxtNamedPipe");
			QVERIFY2(out.open(QIODevice::ReadWrite),"open failed");
			QxtNamedPipe in("/tmp/QxtNamedPipe");
			QVERIFY2(in.open(QIODevice::ReadOnly),"open failed");

			QSignalSpy spyr(&in, SIGNAL(readyRead()));

                        out.write("hello");
                        QString readall=in.readAll();
                        qDebug()<<"output:"<<readall;
                        QVERIFY2(readall=="hello","output not mathing input");
			QVERIFY2 (spyr.count()> 0, "not emitting readyRead" );
			}

	signals:
		void wave();
		void counterwave();
 };



QTEST_MAIN(QxtnamedPipeTest)
#include "main.moc"

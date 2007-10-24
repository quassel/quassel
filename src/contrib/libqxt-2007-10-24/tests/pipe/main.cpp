#include <QxtPipe>
#include <QTest>
#include <QDebug>
#include <QByteArray>
#include <QDataStream>

class QxtPipeTest: public QObject
{
Q_OBJECT 
private slots:
    void simple()
    {
        QxtPipe p1;
        QxtPipe p2;
        p1|p2;
        p1.write("hi");
        QVERIFY(p2.readAll()=="hi");
    }
    void chain()
    {
        QxtPipe p1;
        QxtPipe p2;
        QxtPipe p3;
        p1|p2;
        p2|p3;
        p1.write("hi");
        QVERIFY(p3.readAll()=="hi");
    }
    void bidirectional()
    {
        QxtPipe p1;
        QxtPipe p2;
        p1|p2;

        p1.write("hi");
        QVERIFY(p2.readAll()=="hi");
        QVERIFY(p1.bytesAvailable()==0);

        p2.write("rehi");
        QVERIFY(p1.readAll()=="rehi");
        QVERIFY(p2.bytesAvailable()==0);
    }
    void readOnly()
    {
        QxtPipe p1;
        QxtPipe p2;
        p1.connect(&p2,QIODevice::ReadOnly);

        p1.write("hi");
        QVERIFY(p1.bytesAvailable()==0);
        QVERIFY(p2.bytesAvailable()==0);

        p2.write("rehi");
        QVERIFY(p1.readAll()=="rehi");
        QVERIFY(p2.bytesAvailable()==0);
    }
    void writeOnly()
    {
        QxtPipe p1;
        QxtPipe p2;
        p1.connect(&p2,QIODevice::WriteOnly);

        p1.write("hi");
        QVERIFY(p1.bytesAvailable()==0);
        QVERIFY(p2.readAll()=="hi");

        p2.write("rehi");
        QVERIFY(p1.bytesAvailable()==0);
        QVERIFY(p2.bytesAvailable()==0);
    }
};

QTEST_MAIN(QxtPipeTest)
#include "main.moc"

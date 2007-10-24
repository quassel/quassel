/** ***** QxtJob test ***** */
#include <QTest>
#include <QThread>
class Q43Thread : public QThread{public:void run(){exec();}}; /// qt < 4.3 backwards compatibility

#include <QSignalSpy>
#include <QxtJob>
#include <qxtsignalwaiter.h>



class TestJob : public QxtJob
{
public:
    bool b;
    TestJob():QxtJob()
    {
        b=false;
    }
    virtual void run()
    {
        qDebug("job on on %p",QThread::currentThread ());
        b=true;
    }
};

class QxtJobTest : public QObject
{
Q_OBJECT
private:
    Q43Thread t;
private slots:
    void initTestCase()
    {
        t.start();
    }

    void lined()
    {
        
        TestJob l;
        QSignalSpy spy(&l, SIGNAL(done()));
        QxtSignalWaiter w(&l,SIGNAL(done()));

        l.exec(&t);

        QVERIFY(w.wait(50));
        QCOMPARE(spy.count(), 1);
        QVERIFY(l.b);
    }

    void joined()
    {
        TestJob l;
        l.exec(&t);
        QxtSignalWaiter w(&l,SIGNAL(done()));
        l.join();
        QVERIFY(w.wait(100));
        QVERIFY(l.b);
    }

    void cleanupTestCase()
    {
        t.quit();
        QVERIFY(t.wait(50));
    }
};


int main(int argc, char ** argv)
{
    QCoreApplication app(argc,argv);
    QxtJobTest test1;
    return QTest::qExec(&test1,argc,argv);
}


#include "main.moc"

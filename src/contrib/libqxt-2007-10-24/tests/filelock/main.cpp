/** ***** QxtFileLock test ***** */
#include <QTest>
#include <QFile>
#include <QxtFileLock>
#include <QxtJob>
#include <QMutex>
#include <QWaitCondition>

class QxtFileLockTest: public QObject
{
    Q_OBJECT
private:
    QFile * file1;
    QFile * file2;


private slots:
    void initTestCase()
    {
        file1=new QFile("lock.file");
        file2=new QFile("lock.file");
        QVERIFY(file1->open(QIODevice::ReadWrite));
        QVERIFY(file2->open(QIODevice::ReadWrite));
    }


    ///read and write lock on the same handle
    void rw_same()
    {
        QxtFileLock lock1(file1,0x10,20,QxtFileLock::ReadLock);
        QVERIFY(lock1.lock());
        QxtFileLock lock2(file1,0x10,20,QxtFileLock::WriteLock);
        QVERIFY(lock2.lock());
    }

    ///Trying to readlock the same region with DIFFERENT handles
    void rr_different()
    {
        QxtFileLock lock1(file1,0x10,20,QxtFileLock::ReadLock);
        QxtFileLock lock2(file2,0x10,20,QxtFileLock::ReadLock);
        QVERIFY(lock1.lock() && lock2.lock());
    }

    ///Trying to lock the same region with DIFFERENT handles and different locks
    void rw_different()
    {
        QxtFileLock lock1(file1,0x10,20,QxtFileLock::ReadLock);
        QxtFileLock lock2(file2,0x10,20,QxtFileLock::WriteLock);
        QVERIFY(lock1.lock() && !lock2.lock());
    }

    ///Trying to writelock the same region with DIFFERENT handles
    void ww_different()
    {
        QxtFileLock lock1(file1,0x10,20,QxtFileLock::WriteLock);
        QxtFileLock lock2(file2,0x10,20,QxtFileLock::WriteLock);
        QVERIFY(lock1.lock() && !lock2.lock());
    }

    ///Trying to writelock the different regions with DIFFERENT handles
    void ww_different_region()
    {
        QxtFileLock lock1(file1,0x10   ,20,QxtFileLock::WriteLock);
        QxtFileLock lock2(file2,0x10+21,20,QxtFileLock::WriteLock);
        QVERIFY(lock1.lock() && lock2.lock());
    }

    ///different region, different handles, different locks
    void rw_different_region()
    {
        QxtFileLock lock1(file1,0x10   ,20,QxtFileLock::ReadLock);
        QxtFileLock lock2(file2,0x10+21,20,QxtFileLock::WriteLock);
        QVERIFY(lock1.lock() && lock2.lock());
    }
    ///different region, same handles, different locks
    void rw_same_region()
    {
        QxtFileLock lock1(file1,0x10   ,20,QxtFileLock::ReadLock);
        QxtFileLock lock2(file1,0x10+21,20,QxtFileLock::WriteLock);
        QVERIFY(lock1.lock() && lock2.lock());
    }
    void cleanupTestCase()
    {
        delete file1;
        delete file2;
    }

};
#include <QThread>
#include <qxtsignalwaiter.h>

class Q43Thread : public QThread
{
public:
    void run()
    {
        exec();
    }
}
; /// qt < 4.3 backwards compatibility

///this is a job hack, not part of the testcase, ignore it if you don't what it is

///here is the interesting part of the job. this executes one lock on a spefic thread and asserts the result
class LockJob : public QxtJob
{
public:
    LockJob(QxtFileLock*f,bool expectedresult):QxtJob()
    {
        lock =f;
        expected=expectedresult;
    }
    QxtFileLock*lock ;
    bool expected;
    virtual void run()
    {
        qDebug("locking on %p",QThread::currentThread ());
        QVERIFY(lock ->lock ()==expected);
    }
    void exec(QThread * o)
    {
         QxtJob::exec(o);
         join();
     }
};


class QxtFileLockThreadTest : public QObject
{
    Q_OBJECT
private:
    Q43Thread t1;
    Q43Thread t2;

private slots:
    void initTestCase()
    {
        qDebug("main thread is %p",QThread::currentThread ());

        QxtSignalWaiter w1(&t1,SIGNAL(started()));
        t1.start();
        QVERIFY(t1.isRunning());
        t2.start();
        QVERIFY(t2.isRunning());
    }


    ///Trying to writelock the same region twice
    void ww_same()
    {
        QFile file1("lock.file");
        QVERIFY(file1.open(QIODevice::ReadWrite));
        QFile file2("lock.file");
        QVERIFY(file2.open(QIODevice::ReadWrite));

        QxtFileLock lock1(&file1,0x10,20,QxtFileLock::WriteLock);
        file1.moveToThread(&t1);
        LockJob l(&lock1,true);
        l.exec(&t1);

        QxtFileLock lock2(&file2,0x10,20,QxtFileLock::WriteLock);
        file2.moveToThread(&t2);
        LockJob l2(&lock2,false);
        l2.exec(&t2);
        l2.join();
    }


    ///Trying to readlock the same region
    void rr_same()
    {
        QFile file1("lock.file");
        QVERIFY(file1.open(QIODevice::ReadWrite));
        QFile file2("lock.file");
        QVERIFY(file2.open(QIODevice::ReadWrite));

        QxtFileLock lock1(&file1,0x10,20,QxtFileLock::ReadLock);
        file1.moveToThread(&t1);
        LockJob l1(&lock1,true);
        l1.exec(&t1);
        l1.join();

        QxtFileLock lock2(&file2,0x10,20,QxtFileLock::ReadLock);
        file2.moveToThread(&t2);
        LockJob l2(&lock2,true);
        l2.exec(&t2);
        l2.join();
    }

    ///Trying to lock the same region with different locks
    void rw_same()
    {

        QFile file1("lock.file");
        QVERIFY(file1.open(QIODevice::ReadWrite));
        QFile file2("lock.file");
        QVERIFY(file2.open(QIODevice::ReadWrite));

        QxtFileLock lock1(&file1,0x10,20,QxtFileLock::WriteLock);
        file1.moveToThread(&t1);
        LockJob(&lock1,true).exec(&t1);

        QxtFileLock lock2(&file2,0x10,20,QxtFileLock::ReadLock);
        file2.moveToThread(&t2);
        LockJob(&lock2,false).exec(&t2);
    }

    ///Trying to writelock different regions
    void ww_different()
    {
        QFile file1("lock.file");
        QVERIFY(file1.open(QIODevice::ReadWrite));
        QFile file2("lock.file");
        QVERIFY(file2.open(QIODevice::ReadWrite));

        QxtFileLock lock1(&file1,0x10,20,QxtFileLock::WriteLock);
        file1.moveToThread(&t1);
        LockJob(&lock1,true).exec(&t1);

        QxtFileLock lock2(&file2,0x10+21,20,QxtFileLock::WriteLock);
        file2.moveToThread(&t2);
        LockJob(&lock2,true).exec(&t2);
    }
    void cleanupTestCase()
    {
        t1.quit ();
        t1.wait ();
        t2.quit ();
        t2.wait ();
    }
};


int main(int argc, char ** argv)
{
    QCoreApplication app(argc,argv);
    QxtFileLockTest test1;
    QxtFileLockThreadTest test2;
    return QTest::qExec(&test1,argc,argv)+QTest::qExec(&test2,argc,argv);
}


#include "main.moc"

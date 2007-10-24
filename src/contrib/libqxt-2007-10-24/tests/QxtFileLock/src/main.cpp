

#include <QCoreApplication>
#include <QFile>
#include <QxtFileLock>
#include <QDebug>

#include <windows.h>


#include "threadtestcontroller.h"
#include "HelperClass.h"


/*
Needed Test:
1.    one thread test
       - open the same file twice
       - lock fileregion with readlock (handle 1)
       - try to lock the same region with a read lock -> should work
       - try to lock the same region with a write lock -> should fail
       - remove all locks
       - create a writelock on handle 1
       - create a writelock on handle 2 ->>fail
       - try to lock totally different regions --> should work
       
2.    multiple threadstest:
       - spawn two threads
       - open the same file twice
       - let thread 1 lock (READLOCK) a region of a file
       - let thread 2 do the same lock ---> should work
       - let thread 1 upgrade its lock to a WRITELOCK -->should fail (because thread 2 holds the readlock)
       - remove all locks
       - try to lock totally different regions of the file -> should work
*/

int main(int argc, char *argv[])
{
      QCoreApplication app(argc, argv);
      
      if(1)
      {
        QFile file1("lock.file");
        QFile file2("lock.file");
        
        if(file1.open(QIODevice::ReadWrite) && file2.open(QIODevice::ReadWrite))
        {

            if(1)
            {
                qDebug()<<"----Starting first test----";
                qDebug()<<"Trying to create some locks without collison";

                QxtFileLock lock1(&file1,0x10,20,QxtFileLock::WriteLock);
                if(lock1.lock())
                    qDebug()<<"---- Write Lock Test passed----";
                else
                    qDebug()<<"---- Write Lock Test failed----";

                lock1.unlock();

                QxtFileLock lock2(&file2,0x10,20,QxtFileLock::ReadLock);
                if(lock2.lock())
                    qDebug()<<"---- Read Lock Test passed----";
                else
                    qDebug()<<"---- Read Lock Test failed----";

                lock2.unlock();

            }
            
            if(1)
            {
                qDebug()<<"----Starting next test-----";
                qDebug()<<"Trying to readlock the same region with DIFFERENT handles ";
                QxtFileLock *lock1 = new QxtFileLock(&file1,0x10,20,QxtFileLock::ReadLock);
                QxtFileLock *lock2 = new QxtFileLock(&file2,0x10,20,QxtFileLock::ReadLock);
                            
                if(lock1->lock() && lock2->lock())
                    qDebug()<<"----Test passed----";
                else
                    qDebug()<<"----Test failed----";
                
                delete lock1;
                delete lock2;
            }
            
            if(1)
            {
                qDebug()<<"----Starting next test-----";
                qDebug()<<"Trying to lock the same region with DIFFERENT handles and different locks";
                QxtFileLock *lock1 = new QxtFileLock(&file1,0x10,20,QxtFileLock::ReadLock);
                QxtFileLock *lock2 = new QxtFileLock(&file2,0x10,20,QxtFileLock::WriteLock);
                            
                if(lock1->lock() && !lock2->lock())
                    qDebug()<<"----Test passed----";
                else
                    qDebug()<<"----Test failed----";
                
                delete lock1;
                delete lock2;
            }
            
            if(1)
            {
                qDebug()<<"----Starting next test-----";
                qDebug()<<"Trying to writelock the same region with DIFFERENT handles";
                QxtFileLock *lock1 = new QxtFileLock(&file1,0x10,20,QxtFileLock::WriteLock);
                QxtFileLock *lock2 = new QxtFileLock(&file2,0x10,20,QxtFileLock::WriteLock);
                            
                if(lock1->lock() && !lock2->lock())
                    qDebug()<<"----Test passed----";
                else
                    qDebug()<<"----Test failed----";
                
                delete lock1;
                delete lock2;
            }
            
            if(1)
            {
                qDebug()<<"----Starting next test-----";
                qDebug()<<"Trying to writelock the different regions with DIFFERENT handles";
                QxtFileLock *lock1 = new QxtFileLock(&file1,0x10,20,QxtFileLock::WriteLock);
                QxtFileLock *lock2 = new QxtFileLock(&file2,0x10+21,20,QxtFileLock::WriteLock);
                            
                if(lock1->lock() && lock2->lock())
                    qDebug()<<"----Test passed----";
                else
                    qDebug()<<"----Test failed----";
                
                delete lock1;
                delete lock2;
            }
        }
      }
      
      qDebug()<<"All base tests are finished, now starting the threaded tests";
      
      ThreadTestController controller;
      
      if(controller.startTests())
      {
          HelperClass *testClient = new HelperClass();
          testClient->start();
          return app.exec();    
      }
      
      return 0;
}


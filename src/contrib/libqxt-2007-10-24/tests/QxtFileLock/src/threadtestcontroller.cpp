#include "threadtestcontroller.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

#include <QFile>
#include <QxtFileLock>
#include <QCoreApplication>

ThreadTestController::ThreadTestController(QObject *parent)
 : QObject(parent)
{
    server = new QTcpServer(this);
    connect(server,SIGNAL(newConnection ()),this,SLOT(doTests()));
}


ThreadTestController::~ThreadTestController()
{
}

void ThreadTestController::doTests()
{
    QTcpSocket *socket = server->nextPendingConnection();
    QFile file("lock.file");
    
    #define DoNextTest()             socket->putChar('n');\
                                                    if(socket->waitForReadyRead(-1))\
                                                    {\
                                                        socket->getChar(&testResult);\
                                                        if(testResult == 'f')\
                                                            qDebug()<<"----Test failed----";\
                                                        else if(testResult == 's')\
                                                            qDebug()<<"----Test passed----";\
                                                        else\
                                                            qDebug()<<"----Wrong result value----";\
                                                    }\
                                                    else qDebug()<<"No ready read";
    
    if(socket  && file.open(QIODevice::ReadWrite))
    {
        char testResult = 'f';
        
        if(1)
        {
            qDebug()<<"----Starting next test-----";
            qDebug()<<"Trying to readlock the same region ";
            QxtFileLock lock(&file,0x10,20,QxtFileLock::ReadLock);
            if(lock.lock())
            {
                    DoNextTest();
            }
        }
        
        if(1)
        {
            qDebug()<<"----Starting next test-----";
            qDebug()<<"Trying to lock the same region with different locks";
            QxtFileLock lock(&file,0x10,20,QxtFileLock::ReadLock);
            if(lock.lock())
            {
                DoNextTest();
            }
        }
        
        if(1)
        {
            qDebug()<<"----Starting next test-----";
            qDebug()<<"Trying to writelock the same region twice";
            QxtFileLock lock(&file,0x10,20,QxtFileLock::WriteLock);
            if(lock.lock())
            {
                DoNextTest();
            }
        }
        
        if(1)
        {
            qDebug()<<"----Starting next test-----";
            qDebug()<<"Trying to writelock different regions";
            QxtFileLock lock(&file,0x10,20,QxtFileLock::WriteLock);
            if(lock.lock())
            {
                DoNextTest();
            }
        }
        
    }
    QCoreApplication::instance()->exit();
}

bool ThreadTestController::startTests()
{
    if (!server->listen(QHostAddress::Any,55555))
    {
            qDebug()<<"Could not start listening Server "<<server->serverError();
        return false;
    }
    return true;
}



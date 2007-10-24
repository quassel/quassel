#include "locktestclient.h"
#include <QTimer>
#include <QTcpSocket>
#include <QDebug>
#include <QFile>
#include <QxtFileLock>

LockTestClient::LockTestClient(QObject *parent)
 : QObject(parent)
{
}


LockTestClient::~LockTestClient()
{
}

void LockTestClient::startTests()
{
    QTcpSocket socket;
    socket.connectToHost ( "localhost", 55555);
    char control;
    
    #define GetNextCommand()      if(socket.waitForReadyRead (-1) )\
                                                        {\
                                                            if(socket.bytesAvailable() > 1)\
                                                                qDebug()<<"Something is wrong here";\
                                                            socket.getChar(&control);\
                                                            if(control == 'a')\
                                                            {\
                                                                socket.disconnectFromHost();\
                                                                return;\
                                                            }\
                                                            if(control != 'n')\
                                                            { \
                                                                 qDebug()<<"Wrong control command";\
                                                            }\
                                                        }
    
    if(socket.waitForConnected (-1))
    {
        QFile file("lock.file");
        
        if(!file.open(QIODevice::ReadWrite))
        {
            qDebug()<<"Could not open lockfile";
            return;
        }
        
        if(1)
        {
            GetNextCommand();
            //Trying to readlock the same region
            QxtFileLock lock(&file,0x10,20,QxtFileLock::ReadLock);
            if(lock.lock())
                socket.putChar('s');    //s for success f for fail
            else
                socket.putChar('f');
            socket.waitForBytesWritten(-1);
        }
        
        if(1)
        {
            GetNextCommand();
             //Trying to lock the same region with different locks
            QxtFileLock lock(&file,0x10,20,QxtFileLock::WriteLock); 
            
            if(!lock.lock())
                socket.putChar('s');    //s for success f for fail
            else
                socket.putChar('f');
            socket.waitForBytesWritten(-1);
        }
        
        if(1)
        {
            GetNextCommand();
             //Trying to writelock the same region
            QxtFileLock lock(&file,0x10,20,QxtFileLock::WriteLock); 
            
            if(!lock.lock())
                socket.putChar('s');    //s for success f for fail
            else
                socket.putChar('f');
            socket.waitForBytesWritten(-1);
        }
        
        if(1)
        {
            GetNextCommand();
             //Trying to writelock different regions
            QxtFileLock lock(&file,0x10+21,20,QxtFileLock::WriteLock); 
            
            if(lock.lock())
                socket.putChar('s');    //s for success f for fail
            else
                socket.putChar('f');
            socket.waitForBytesWritten(-1);
        }
        
    }
}



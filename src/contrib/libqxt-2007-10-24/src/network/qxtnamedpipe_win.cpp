#include <windows.h>
#include <io.h>
#include <QSocketNotifier>
#include <QByteArray>
#include "qxtnamedpipe.h"

#define BUFSIZE 4096
#define PIPE_TIMEOUT  (120*1000) /*120 seconds*/

#include "qxtnamedpipe_win_p.h"

void QxtNamedPipePrivate::bytesAvailable()
{
    char chBuf[BUFSIZE];
    bool fSuccess = false;
    DWORD cbRead = 0;

    do
    {
        // Read from the pipe.

        memset((void *)chBuf,0,BUFSIZE*sizeof(char));

        fSuccess = ReadFile(
                       this->win32Handle,      // pipe handle
                       chBuf,                  // buffer to receive reply
                       BUFSIZE*sizeof(char),  // size of buffer
                       &cbRead,                // number of bytes read
                       NULL);                  // not overlapped

        if (! fSuccess && GetLastError() != ERROR_MORE_DATA)
            break;

        this->readBuffer.append(QByteArray(chBuf,cbRead));

    }
    while (!fSuccess);  // repeat loop if ERROR_MORE_DATA

    emit readyRead();
}

QxtNamedPipe::QxtNamedPipe(const QString& name, QObject* parent) : QIODevice(parent)
{
    QXT_INIT_PRIVATE(QxtNamedPipe);
    qxt_d().pipeName = name;
    qxt_d().win32Handle = INVALID_HANDLE_VALUE;
    qxt_d().fd = -1;
    qxt_d().serverMode = false;
    qxt_d().notify = 0;

}

bool QxtNamedPipe::open(QIODevice::OpenMode mode)
{
    int m = PIPE_ACCESS_DUPLEX;
    int m_client = GENERIC_READ | GENERIC_WRITE;

    if (!(mode & QIODevice::ReadOnly))
    {
        m = PIPE_ACCESS_OUTBOUND;
        m_client = GENERIC_WRITE;
    }
    else if (!(mode & QIODevice::WriteOnly))
    {
        m = PIPE_ACCESS_INBOUND;
        m_client = GENERIC_READ;
    }

    QString pipePrefix("\\\\.\\pipe\\");

    // first try to open in client mode
    qxt_d().win32Handle  = CreateFileA(qPrintable(pipePrefix+qxt_d().pipeName),   // pipe name
                                       m_client,                      // read and write access
                                       0,                             // no sharing
                                       NULL,                          // default security attributes
                                       OPEN_EXISTING ,                // opens a pipe
                                       0,                             // default attributes
                                       NULL);                         // no template file

    //if we have no success create the pipe and open it
    if (qxt_d().win32Handle == NULL || qxt_d().win32Handle == INVALID_HANDLE_VALUE)
    {
        qxt_d().win32Handle  =   CreateNamedPipeA(qPrintable(pipePrefix+qxt_d().pipeName),        //pipe Name must be \\.\pipe\userdefinedname
                                 m,                                //read/write mode
                                 PIPE_NOWAIT,                      //don't block
                                 1,                                //max number of instances 1
                                 BUFSIZE,                          //ouput buffer size allocate as needed
                                 BUFSIZE,                          //input buffer size allocate as needed
                                 PIPE_TIMEOUT,                     //default timeout value
                                 NULL);                            //security attributes
        if (qxt_d().win32Handle != NULL && qxt_d().win32Handle != INVALID_HANDLE_VALUE)
            qxt_d().serverMode = true;
    }
    else
    {
        qxt_d().serverMode = false;

        DWORD pipeMode = PIPE_NOWAIT;
        SetNamedPipeHandleState(
            qxt_d().win32Handle,    // pipe handle
            &pipeMode,              // new pipe mode
            NULL,                   // don't set maximum bytes
            NULL);                  // don't set maximum time

    }


    if (qxt_d().win32Handle != NULL && qxt_d().win32Handle != INVALID_HANDLE_VALUE)
    {
        qxt_d().fd = _open_osfhandle((long)qxt_d().win32Handle,0); //FIXME that is not x64 compatible
        setOpenMode ( mode);

        if (!qxt_d().notify)
            qxt_d().notify = new QSocketNotifier(qxt_d().fd,QSocketNotifier::Read,this);

        qxt_d().notify->setEnabled(true);
        connect(qxt_d().notify,SIGNAL(activated(int)),&qxt_d(),SLOT(bytesAvailable()));
        connect(&qxt_d(),SIGNAL(readyRead()),this,SIGNAL(readyRead()));

        return true;
    }
    else
    {
        return false;
    }
}

bool QxtNamedPipe::open(const QString& name, QIODevice::OpenMode mode)
{
    qxt_d().pipeName = name;
    return QxtNamedPipe::open(mode);
}

void QxtNamedPipe::close()
{
    if (qxt_d().win32Handle != NULL && qxt_d().win32Handle != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(qxt_d().win32Handle);
        if (qxt_d().serverMode)
        {
            DisconnectNamedPipe(qxt_d().win32Handle);
        }

        qxt_d().notify->setEnabled(false);
        delete qxt_d().notify;
        qxt_d().notify = 0;
        qxt_d().readBuffer.clear();

        //this will close native and C handle
        _close(qxt_d().fd);
        qxt_d().win32Handle = INVALID_HANDLE_VALUE;
        qxt_d().fd = -1;
    }
    setOpenMode(QIODevice::NotOpen);
}

QByteArray QxtNamedPipe::readAvailableBytes()
{
    char ch;
    QByteArray rv;
    while (getChar(&ch)) rv += ch;
    return rv;
}

qint64 QxtNamedPipe::bytesAvailable () const
{
    return qxt_d().readBuffer.size();
}

qint64 QxtNamedPipe::readData ( char * data, qint64 maxSize )
{
    qint64 toRead = qxt_d().readBuffer.size() < maxSize ? qxt_d().readBuffer.size() : maxSize;

    memcpy(data,qxt_d().readBuffer.data(),toRead);
    qxt_d().readBuffer.remove(0,toRead);

    return toRead;

}

qint64 QxtNamedPipe::writeData ( const char * data, qint64 maxSize )
{
    DWORD bytesWritten = 0;

    bool fSuccess = WriteFile(
                        qxt_d().win32Handle,    // pipe handle
                        data,                   // message
                        maxSize,                // message length
                        &bytesWritten,          // bytes written
                        NULL);                  // not overlapped
    if (!fSuccess)
    {
        return -1;
    }
    return bytesWritten;
}

bool QxtNamedPipe::isSequential () const
{
    return true;
}


#ifndef QXTNAMEDPIPEPRIVATE_WIN_H_INCLUDED
#define QXTNAMEDPIPEPRIVATE_WIN_H_INCLUDED

#include <QString>
#include <QObject>
#include <QByteArray>
#include <windows.h>
#include "qxtpimpl.h"

class QSocketNotifier;
class QxtNamedPipe;

class QxtNamedPipePrivate : public QObject, public QxtPrivate<QxtNamedPipe>
{
    Q_OBJECT
public:
    QxtNamedPipePrivate()
    {}
    QXT_DECLARE_PUBLIC(QxtNamedPipe);
    QString pipeName;
    HANDLE win32Handle;
    int fd;
    bool serverMode;
    QSocketNotifier * notify;
    QByteArray readBuffer;

signals:
    void readyRead();

public slots:
    void bytesAvailable();

};

#endif


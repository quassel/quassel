#ifndef QXTNAMEDPIPE_H
#define QXTNAMEDPIPE_H

#include <QIODevice>
#include <QString>
#include <QByteArray>
#include <qxtpimpl.h>
#include <qxtglobal.h>

class QxtNamedPipePrivate;

class QXT_NETWORK_EXPORT QxtNamedPipe : public QIODevice
{
    Q_OBJECT

public:
    QxtNamedPipe(const QString& name = QString(), QObject* parent = 0);
    virtual bool isSequential () const;

    virtual qint64 bytesAvailable () const;
    virtual qint64 readData ( char * data, qint64 maxSize );
    virtual qint64 writeData ( const char * data, qint64 maxSize );

    bool open(QIODevice::OpenMode mode);
    bool open(const QString& name, QIODevice::OpenMode mode);
    void close();

    QByteArray readAvailableBytes();

private:
    QXT_DECLARE_PRIVATE(QxtNamedPipe);
};

#endif

#include "QxtNamedPipe.h"
#ifdef Q_OS_UNIX
#    include <fcntl.h>
#else
#    error "No Windows implementation for QxtNamedPipe"
#endif

class QxtNamedPipePrivate : public QxtPrivate<QxtNamedPipe>
{
public:
    QxtNamedPipePrivate();
    QXT_DECLARE_PUBLIC(QxtNamedPipe);

    QString pipeName;
    int fd;
};

QxtNamedPipe::QxtNamedPipe(const QString& name, QObject* parent) : QAbstractSocket(QAbstractSocket::UnknownSocketType, parent)
{
    QXT_INIT_PRIVATE(QxtNamedPipe);
    qxt_d().pipeName = name;
    qxt_d().fd = 0;
}

bool QxtNamedPipe::open(QIODevice::OpenMode mode)
{
    int m = O_RDWR;
    if (!(mode & QIODevice::ReadOnly)) m = O_WRONLY;
    else if (!(mode & QIODevice::WriteOnly)) m = O_RDONLY;
    qxt_d().fd = ::open(qPrintable(qxt_d().pipeName), m);
    return (qxt_d().fd != 0);
}

bool QxtNamedPipe::open(const QString& name, QIODevice::OpenMode mode)
{
    qxt_d().pipeName = name;
    return QxtNamedPipe::open(mode);
}

void QxtNamedPipe::close()
{
    if (qxt_d().fd) ::close(qxt_d().fd);
}

QByteArray QxtNamedPipe::readAvailableBytes()
{
    char ch;
    QByteArray rv;
    while (getChar(&ch)) rv += ch;
    return rv;
}

#ifndef LOCKTESTCLIENT_H
#define LOCKTESTCLIENT_H

#include <QObject>

/**
	@author Benjamin Zeller <zbenjamin@libqxt.org>
*/

class QTcpSocket;

class LockTestClient : public QObject
{
    Q_OBJECT
    public:
        LockTestClient(QObject *parent = 0);
        ~LockTestClient();
    
    public slots:
        void startTests();
        
    private:
        QTcpSocket *socket;

};

#endif

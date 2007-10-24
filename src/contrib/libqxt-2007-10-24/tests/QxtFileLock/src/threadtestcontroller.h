#ifndef THREADTESTCONTROLLER_H
#define THREADTESTCONTROLLER_H

#include <QObject>

/**
	@author Benjamin Zeller <zbenjamin@libqxt.org>
*/


/**
 * the test controller controls the client over a QTcpSocker port 55555
 * there are some controll commands
 * n : start next test
 * a : abort testing
 * from the client there are two possible answers
 * s: the test succeeded
 * f: the test failed
 */

class QTcpServer;

class ThreadTestController : public QObject
{
    Q_OBJECT
    public:
        ThreadTestController(QObject *parent = 0);
        ~ThreadTestController();
        bool startTests();
        
    public slots:
        void doTests();
        
    private:
        QTcpServer *server;

};

#endif

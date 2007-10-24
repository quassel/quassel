#ifndef HELPERCLASS_H
#define HELPERCLASS_H

#include <QThread>

/**
	@author Benjamin Zeller <zbenjamin@libqxt.org>
*/
class HelperClass : public QThread
{
Q_OBJECT
public:
    HelperClass(QObject *parent = 0);
    ~HelperClass();
    void run();

};

#endif

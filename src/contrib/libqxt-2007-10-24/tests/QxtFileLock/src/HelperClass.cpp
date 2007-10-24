#include "HelperClass.h"
#include "locktestclient.h"

HelperClass::HelperClass(QObject *parent)
 : QThread(parent)
{
}


HelperClass::~HelperClass()
{
}

void HelperClass::run()
{
    QObject threadParent;
    LockTestClient *client = new LockTestClient(&threadParent);
    client->startTests();
    exec();
}



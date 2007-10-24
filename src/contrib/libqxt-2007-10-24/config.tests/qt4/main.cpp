#include <QObject>


#if QT_VERSION < 0x040000
#error needs qt4
#endif


int main (int,char**)
	{
        QObject();
	return 0;
	}



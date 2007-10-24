#include <QxtApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
	QxtApplication a(argc, argv);
	MainWindow w;
	w.show();
	return a.exec();
}

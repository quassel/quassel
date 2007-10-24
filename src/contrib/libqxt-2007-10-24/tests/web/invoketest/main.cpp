#include <QCoreApplication>
#include <QxtWebCore>
#include <QxtWebController>
#include <QxtScgiConnector>
class test : public QxtWebController
        {
        Q_OBJECT
        public:
                test():QxtWebController("root")
			{
			}

	public slots:

		 int index()
                        {
			echo()<<"foo";
                        return 0;
                        }
		 int index(QString a, QString b=QString("default"), QString c=QString("default2"))
                        {
			echo()<<"a: "<<a<<"<br/> b: "<<b<<"<br/> c: "<<c;
                        return 0;
                        }

        };

class err : public QxtWebController
        {
        Q_OBJECT
        public:
                err():QxtWebController("error")
			{
			}

	public slots:

		 int index(QString a=QString("500"),QString b=QString(),QString c=QString(),QString d=QString(),
			QString e=QString(),QString f=QString(),QString g=QString())
                        {
			echo()<<"shits<br/>"
			"error "<<a<<"<br/>"
			<<b<<"<br/>"
			<<c<<"<br/>"
			<<d<<"<br/>"
			<<e<<"<br/>"
			<<f<<"<br/>"
			<<g<<"<br/>";
                        return 0;
                        }

        };





int main(int argc, char *argv[])
	{
	QCoreApplication app(argc, argv);
	QxtWebCore core(new QxtScgiConnector());
	core.start(4000);
	test t;
	err e;
	return app.exec();
	}


#include "main.moc"

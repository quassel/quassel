#include <QCoreApplication>
#include <QxtWebCore>
#include <QxtWebController>
#include <QxtFcgiConnector>
#include <QTimer>
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
        echo()<<
            "<form method='POST' enctype='multipart/form-data' action='/root/upload'>"
            "File to upload: <input type=file name=upfile><br>"
            "Notes about the file: <input type=text name=note><br>"
            "<br>"
            "<input type=submit value=Press> to upload the file!"
            "</form>";
        return 0;
    }
    int upload()
    {

        QByteArray d= QxtWebCore::content(100000000);
        QxtWebCore::sendHeader();

        QByteArray io;
        QxtWebCore::socket()->write(d);
        return 0;
    }
};





int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QxtWebCore core(new QxtFcgiConnector());
    core.start();
    test t;
    return app.exec();
}


#include "main.moc"

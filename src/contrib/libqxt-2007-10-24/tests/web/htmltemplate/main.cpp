#include <QTest>
#include <QSignalSpy>
#include <QxtHtmlTemplate>
class Test: public QObject
{
Q_OBJECT 
private slots:
    void simple()
    { 
        QxtHtmlTemplate t;
        t.load("<?=foo?>");
        t["foo"]="bla";
        QVERIFY(t.render()=="bla");
    }
    void surounded()
    { 
        QxtHtmlTemplate t;
        t.load("123456789<?=foo?>123456789");
        t["foo"]="heyJO";
        QVERIFY(t.render()=="123456789heyJO123456789");
    }
    void indented()
    { 
        QxtHtmlTemplate t;
        t.load("\n       <?=foo?>");
        t["foo"]="baz\nbar";
        QVERIFY(t.render()=="\n       baz\n       bar");
    }
};

QTEST_MAIN(Test)
#include "main.moc"

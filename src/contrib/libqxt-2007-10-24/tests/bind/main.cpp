#include <QTest>
#include <QObject>
#include <qxtmetaobject.h>
#include <qxtboundcfunction.h>
#include <QSignalSpy>

void unaryVoidFunction(QObject* obj);

void nullaryVoidFunction() {
    qDebug() << "If you don't see this, it's broken.";
}

int nullaryIntFunction()
{
    return 5;
}

int unaryIntFunction(int x)
{
    return x * 2;
}

class QxtMetaObjectTest: public QObject
{
    Q_OBJECT 

signals:
    void say(QString);
    void doit();
    void success();

public:
    void unaryVoidFunctionSuccess() {
        emit success();
    }

private slots:
    void readwrite()
    {
        QxtMetaObject::connect(this, SIGNAL(doit()), QxtMetaObject::bind(this, SLOT(say(QString)), Q_ARG(QString,"hello")));
        QSignalSpy spy(this, SIGNAL(say(QString)));
        emit(doit());
        QVERIFY2 (spy.count()> 0, "no signal received" );
        QVERIFY2 (spy.count()< 2, "wtf, two signals received?" );

        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY2(arguments.at(0).toString()=="hello","argument missmatch");

        QxtBoundFunction* nullaryVoid = QxtMetaObject::bind<void>(qxtFuncPtr(nullaryVoidFunction));
        QxtBoundFunction* unaryVoid = QxtMetaObject::bind<void, QObject*>(qxtFuncPtr(unaryVoidFunction), Q_ARG(QObject*, this));
        QxtBoundFunction* nullaryInt = QxtMetaObject::bind<int>(qxtFuncPtr(nullaryIntFunction));
        QxtBoundFunction* unaryIntFixed = QxtMetaObject::bind<int, int>(qxtFuncPtr(unaryIntFunction), Q_ARG(int, 7));
        QxtBoundFunction* unaryIntBound = QxtMetaObject::bind<int, int>(qxtFuncPtr(unaryIntFunction), QXT_BIND(1));
        QVERIFY2(nullaryVoid != 0, "nullaryVoidFunction bind failed");
        QVERIFY2(unaryVoid != 0, "unaryVoidFunction bind failed");
        QVERIFY2(nullaryInt != 0, "nullaryIntFunction bind failed");
        QVERIFY2(unaryIntFixed != 0, "unaryIntFunction bind failed with Q_ARG");
        QVERIFY2(unaryIntBound != 0, "unaryIntFunction bind failed with QXT_BIND");

        bool ok;
        ok = nullaryVoid->invoke();
        QVERIFY2(ok, "nullaryVoid invoke failed");

        QSignalSpy spy2(this, SIGNAL(success()));
        ok = unaryVoid->invoke(this);
        QVERIFY2(ok, "unaryVoid invoke failed");
        QVERIFY2(spy2.count() == 1, "unaryVoid did not emit success");

        int v1 = nullaryIntFunction();
        int v2 = nullaryInt->invoke<int>();
        QVERIFY2(v1 == v2, "nullaryInt returned wrong value");

        v1 = unaryIntFunction(7);
        v2 = unaryIntFixed->invoke<int>();
        QVERIFY2(v1 == v2, "unaryIntFixed returned wrong value");

        v1 = unaryIntFunction(12);
        v2 = unaryIntBound->invoke<int>(12);
        QVERIFY2(v1 == v2, "unaryIntBound returned wrong value");
        

    }

};

void unaryVoidFunction(QObject* obj)
{
    QxtMetaObjectTest* o = qobject_cast<QxtMetaObjectTest*>(obj);
    if(!o) return;
    o->unaryVoidFunctionSuccess();
}



QTEST_MAIN(QxtMetaObjectTest)
#include "main.moc"


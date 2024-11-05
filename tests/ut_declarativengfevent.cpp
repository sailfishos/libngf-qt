#include <QtCore/QPointer>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusReply>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtQml/QQmlProperty>

#include "testbase.h"
#include "moc_testbase.cpp"

namespace Ngf {
namespace Tests {

class UtDeclarativeNgfEvent : public TestBase
{
    Q_OBJECT

    // Keep in sync with DeclarativeNgfEvent::EventStatus
    enum EventStatus {
        Stopped,
        Failed,
        Playing,
        Paused
    };

    class DeclarativeExpression;

public:
    UtDeclarativeNgfEvent();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEventProperty();
    void testPlay();
    void testPause();
    void testStop();
    void testStopOutside();
    void testFail();
    void testPlayFail();
    void testConnectionStatus();

private:
    QPointer<QQmlEngine> m_engine;
    QPointer<QQmlComponent> m_component;
    QPointer<QObject> m_instance;
};

class UtDeclarativeNgfEvent::DeclarativeExpression : public QQmlExpression
{
    Q_OBJECT

public:
    DeclarativeExpression(QQmlContext *ctxt, QObject *scope, const QString &expression,
            QObject *parent = 0)
        : QQmlExpression(ctxt, scope, expression, parent)
    {
        connect(this, SIGNAL(valueChanged()), this, SLOT(emitValueChangedWithValue()));
    }

signals:
    void valueChanged(const QVariant &value);

private slots:
    void emitValueChangedWithValue()
    {
        emit valueChanged(evaluate());
    }
};

} // namespace Tests
} // namespace Ngf

using namespace Ngf::Tests;

/*
 * \class Ngf::Tests::UtDeclarativeNgfEvent
 */

UtDeclarativeNgfEvent::UtDeclarativeNgfEvent()
{
}

void UtDeclarativeNgfEvent::initTestCase()
{
    QVERIFY(waitForService(service()));

    m_engine = new QQmlEngine;
    QQmlComponent component(m_engine);
    component.setData(
        "import Nemo.Ngf 1.0\n"
        "NonGraphicalFeedback { id: instance; }",
        QUrl("file:///dev/null"));
    QVERIFY2(component.isReady(),
            qPrintable(QString("Component is not ready: %1").arg(
                    component.isError()
                    ? component.errors().first().toString()
                    : "Unknown error")));
    m_instance = component.create();

    QVERIFY(m_instance != 0);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), false);
}

void UtDeclarativeNgfEvent::cleanupTestCase()
{
    delete m_instance;
    delete m_engine;
}

void UtDeclarativeNgfEvent::testEventProperty()
{
    QQmlProperty eventProperty(m_instance, "event");

    QQmlExpression eventExpression(m_engine->rootContext(), m_instance, "event");
    eventExpression.setNotifyOnValueChanged(true);
    eventExpression.evaluate();
    QVERIFY2(!eventExpression.hasError(), qPrintable(eventExpression.error().toString()));
    SignalSpy eventSpy(&eventExpression, SIGNAL(valueChanged()));

    QVERIFY(eventProperty.write("an-event"));

    QVERIFY(waitForSignal(&eventSpy));
    QCOMPARE(eventSpy.count(), 1);

    QCOMPARE(eventProperty.read().toString(), QString("an-event"));
    QCOMPARE(eventExpression.evaluate().toString(), QString("an-event"));

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), false);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Stopped);
}

void UtDeclarativeNgfEvent::testPlay()
{
    QDBusInterface client(service(), path(),interface(), bus());

    QQmlExpression statusExpression(m_engine->rootContext(), m_instance, "status");
    statusExpression.setNotifyOnValueChanged(true);
    statusExpression.evaluate();
    QVERIFY2(!statusExpression.hasError(), qPrintable(statusExpression.error().toString()));
    SignalSpy statusSpy(&statusExpression, SIGNAL(valueChanged()));

    SignalSpy playCalledSpy(&client, SIGNAL(mock_playCalled(QString,QVariantMap)));

    QQmlExpression playExpression(m_engine->rootContext(), m_instance, "play()");
    playExpression.evaluate();
    QVERIFY2(!playExpression.hasError(), qPrintable(playExpression.error().toString()));

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignals(SignalSpyList() << &playCalledSpy << &statusSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), QVariantMap());
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Playing);
}

void UtDeclarativeNgfEvent::testPause()
{
    QDBusInterface client(service(), path(),interface(), bus());

    QQmlExpression statusExpression(m_engine->rootContext(), m_instance, "status");
    statusExpression.setNotifyOnValueChanged(true);
    statusExpression.evaluate();
    QVERIFY2(!statusExpression.hasError(), qPrintable(statusExpression.error().toString()));
    SignalSpy statusSpy(&statusExpression, SIGNAL(valueChanged()));

    SignalSpy pauseCalledSpy(&client, SIGNAL(mock_pauseCalled(quint32,bool)));

    QQmlExpression pauseExpression(m_engine->rootContext(), m_instance, "pause()");
    pauseExpression.evaluate();
    QVERIFY2(!pauseExpression.hasError(), qPrintable(pauseExpression.error().toString()));

    QVERIFY(waitForSignals(SignalSpyList() << &pauseCalledSpy << &statusSpy));
    QCOMPARE(pauseCalledSpy.count(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(1).toBool(), true);
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Paused);

    statusSpy.clear();
    pauseCalledSpy.clear();

    QQmlExpression resumeExpression(m_engine->rootContext(), m_instance, "resume()");
    resumeExpression.evaluate();
    QVERIFY2(!resumeExpression.hasError(), qPrintable(resumeExpression.error().toString()));

    QVERIFY(waitForSignals(SignalSpyList() << &pauseCalledSpy << &statusSpy));
    QCOMPARE(pauseCalledSpy.count(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(1).toBool(), false);
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Playing);
}

void UtDeclarativeNgfEvent::testStop()
{
    QDBusInterface client(service(), path(),interface(), bus());

    QQmlExpression statusExpression(m_engine->rootContext(), m_instance, "status");
    statusExpression.setNotifyOnValueChanged(true);
    statusExpression.evaluate();
    QVERIFY2(!statusExpression.hasError(), qPrintable(statusExpression.error().toString()));
    SignalSpy statusSpy(&statusExpression, SIGNAL(valueChanged()));

    SignalSpy stopCalledSpy(&client, SIGNAL(mock_stopCalled(uint)));

    QQmlExpression stopExpression(m_engine->rootContext(), m_instance, "stop()");
    stopExpression.evaluate();
    QVERIFY2(!stopExpression.hasError(), qPrintable(stopExpression.error().toString()));

    QVERIFY(waitForSignals(SignalSpyList() << &stopCalledSpy << &statusSpy));
    QCOMPARE(stopCalledSpy.count(), 1);
    QCOMPARE(stopCalledSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Stopped);
}

void UtDeclarativeNgfEvent::testStopOutside()
{
    QDBusInterface client(service(), path(),interface(), bus());

    QQmlExpression statusExpression(m_engine->rootContext(), m_instance, "status");
    statusExpression.setNotifyOnValueChanged(true);
    statusExpression.evaluate();
    QVERIFY2(!statusExpression.hasError(), qPrintable(statusExpression.error().toString()));
    SignalSpy statusSpy(&statusExpression, SIGNAL(valueChanged()));

    SignalSpy playCalledSpy(&client, SIGNAL(mock_playCalled(QString,QVariantMap)));

    QQmlExpression playExpression(m_engine->rootContext(), m_instance, "play()");
    playExpression.evaluate();
    QVERIFY2(!playExpression.hasError(), qPrintable(playExpression.error().toString()));

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignals(SignalSpyList() << &playCalledSpy << &statusSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), QVariantMap());
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Playing);

    statusSpy.clear();

    client.call("mock_stop", "an-event");

    QVERIFY(waitForSignals(SignalSpyList() << &statusSpy));
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Stopped);
}

void UtDeclarativeNgfEvent::testFail()
{
    QDBusInterface client(service(), path(),interface(), bus());

    QQmlExpression statusExpression(m_engine->rootContext(), m_instance, "status");
    statusExpression.setNotifyOnValueChanged(true);
    statusExpression.evaluate();
    QVERIFY2(!statusExpression.hasError(), qPrintable(statusExpression.error().toString()));
    SignalSpy statusSpy(&statusExpression, SIGNAL(valueChanged()));

    SignalSpy playCalledSpy(&client, SIGNAL(mock_playCalled(QString,QVariantMap)));

    QQmlExpression playExpression(m_engine->rootContext(), m_instance, "play()");
    playExpression.evaluate();
    QVERIFY2(!playExpression.hasError(), qPrintable(playExpression.error().toString()));

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignals(SignalSpyList() << &playCalledSpy << &statusSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), QVariantMap());
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Playing);

    statusSpy.clear();

    client.call("mock_fail", "an-event");

    QVERIFY(waitForSignal(&statusSpy));
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Failed);
}

void UtDeclarativeNgfEvent::testPlayFail()
{
    QDBusInterface client(service(), path(),interface(), bus());

    QQmlExpression statusExpression(m_engine->rootContext(), m_instance, "status");
    statusExpression.setNotifyOnValueChanged(true);
    statusExpression.evaluate();
    QVERIFY2(!statusExpression.hasError(), qPrintable(statusExpression.error().toString()));
    SignalSpy statusSpy(&statusExpression, SIGNAL(valueChanged()));

    SignalSpy playCalledSpy(&client, SIGNAL(mock_playCalled(QString,QVariantMap)));

    client.call("mock_failNextPlay");

    QQmlExpression playExpression(m_engine->rootContext(), m_instance, "play()");
    playExpression.evaluate();
    QVERIFY2(!playExpression.hasError(), qPrintable(playExpression.error().toString()));

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignals(SignalSpyList() << &playCalledSpy << &statusSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), QVariantMap());
    QCOMPARE(statusSpy.count(), 1);

    QCOMPARE(QQmlProperty::read(m_instance, "connected").toBool(), true);
    QCOMPARE(QQmlProperty::read(m_instance, "status").toInt(), (int)Failed);
}

void UtDeclarativeNgfEvent::testConnectionStatus()
{
    QSKIP("Libngf-qt not currently tracking the daemon availability");
    QDBusInterface client(service(), path(), interface(), bus());

    DeclarativeExpression connectedExpression(m_engine->rootContext(), m_instance, "connected");
    connectedExpression.setNotifyOnValueChanged(true);
    connectedExpression.evaluate();
    QVERIFY2(!connectedExpression.hasError(), qPrintable(connectedExpression.error().toString()));
    SignalSpy connectedSpy(&connectedExpression, SIGNAL(valueChanged(QVariant)));

    client.call("mock_disconnectForAWhile");

    QList<bool> expected = QList<bool>() << false << true;
    while (!expected.isEmpty()) {
        QVERIFY(waitForSignal(&connectedSpy));
        while (!connectedSpy.isEmpty()) {
            QCOMPARE(connectedSpy.at(0).at(0).toBool(), expected.at(0));
            connectedSpy.removeAt(0);
            expected.removeAt(0);
        }
    }
}

TEST_MAIN(UtDeclarativeNgfEvent)

#include "ut_declarativengfevent.moc"

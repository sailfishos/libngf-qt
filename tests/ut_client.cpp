#include <QtCore/QPointer>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusReply>

#include "ngfclient.h"

#include "testbase.h"
#include "moc_testbase.cpp"

namespace Ngf {
namespace Tests {

class UtClient : public TestBase
{
    Q_OBJECT

public:
    UtClient();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPlay();
    void testPause();
    void testStop();
    void testFail();
    void testPlayFail();
    void testConnectionStatus();
    void testFastPlayStop();

private:
    QPointer<Client> m_client;
};

} // namespace Tests
} // namespace Ngf

using namespace Ngf::Tests;

/*
 * \class Ngf::Tests::UtClient
 */

UtClient::UtClient()
{
}

void UtClient::initTestCase()
{
    QVERIFY(waitForService(service()));

    m_client = new Client(this);

    SignalSpy connectionStatusSpy(m_client, SIGNAL(connectionStatus(bool)));

    QVERIFY(m_client->connect());

    QVERIFY(waitForSignal(&connectionStatusSpy));
    QCOMPARE(connectionStatusSpy.count(), 1);
    QCOMPARE(connectionStatusSpy.at(0).at(0).toBool(), true);

    QVERIFY(m_client->isConnected());
}

void UtClient::cleanupTestCase()
{
    if (m_client == 0) {
        return;
    }

    SignalSpy connectionStatusSpy(m_client, SIGNAL(connectionStatus(bool)));

    m_client->disconnect();

    QVERIFY(waitForSignal(&connectionStatusSpy));
    QCOMPARE(connectionStatusSpy.count(), 1);
    QCOMPARE(connectionStatusSpy.at(0).at(0).toBool(), false);

    QVERIFY(!m_client->isConnected());

    delete m_client;
}

void UtClient::testPlay()
{
    QDBusInterface client(service(), path(),interface(), bus());

    SignalSpy playCalledSpy(&client, SIGNAL(mock_playCalled(QString,QVariantMap)));

    QVariantMap properties;
    properties["foo"] = "fooval";
    properties["bar"] = 42;

    quint32 id = m_client->play("an-event", properties);
    QVERIFY(id > 0);

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignal(&playCalledSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), properties);
}

void UtClient::testPause()
{
    QDBusInterface mockService(service(), path(), interface(), bus());

    SignalSpy pauseCalledSpy(&mockService, SIGNAL(mock_pauseCalled(quint32,bool)));

    SignalSpy eventPausedSpy(m_client, SIGNAL(eventPaused(quint32)));

    m_client->pause("an-event");

    QVERIFY(waitForSignals(SignalSpyList() << &eventPausedSpy << &pauseCalledSpy));
    QCOMPARE(eventPausedSpy.count(), 1);
    QCOMPARE(eventPausedSpy.at(0).at(0).toUInt(), 1u);
    QCOMPARE(pauseCalledSpy.count(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(1).toBool(), true);

    pauseCalledSpy.clear();

    SignalSpy eventPlayingSpy(m_client, SIGNAL(eventPlaying(quint32)));

    m_client->resume("an-event");

    QVERIFY(waitForSignals(SignalSpyList() << &eventPlayingSpy << &pauseCalledSpy));
    QCOMPARE(eventPlayingSpy.count(), 1);
    QCOMPARE(eventPlayingSpy.at(0).at(0).toUInt(), 1u);
    QCOMPARE(pauseCalledSpy.count(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(pauseCalledSpy.at(0).at(1).toBool(), false);
}

void UtClient::testStop()
{
    QDBusInterface mockService(service(), path(), interface(), bus());

    SignalSpy stopCalledSpy(&mockService, SIGNAL(mock_stopCalled(uint)));

    SignalSpy eventCompletedSpy(m_client, SIGNAL(eventCompleted(quint32)));

    QVERIFY(m_client->stop("an-event"));

    QVERIFY(waitForSignals(SignalSpyList() << &eventCompletedSpy << &stopCalledSpy));
    QCOMPARE(eventCompletedSpy.count(), 1);
    QCOMPARE(eventCompletedSpy.at(0).at(0).toUInt(), 1u);
    QCOMPARE(stopCalledSpy.count(), 1);
    QCOMPARE(stopCalledSpy.at(0).at(0).toInt(), 1);
}

void UtClient::testFail()
{
    QDBusInterface mockService(service(), path(), interface(), bus());

    SignalSpy playCalledSpy(&mockService, SIGNAL(mock_playCalled(QString,QVariantMap)));

    QVariantMap properties;
    properties["foo"] = "fooval";
    properties["bar"] = 42;

    quint32 id = m_client->play("an-event", properties);
    QVERIFY(id > 0);

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignal(&playCalledSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), properties);

    SignalSpy eventFailedSpy(m_client, SIGNAL(eventFailed(quint32)));

    mockService.call("mock_fail", "an-event");

    QVERIFY(waitForSignal(&eventFailedSpy));
    QCOMPARE(eventFailedSpy.count(), 1);
    QCOMPARE(eventFailedSpy.at(0).at(0).toUInt(), 2u);
}

void UtClient::testPlayFail()
{
    QDBusInterface mockService(service(), path(), interface(), bus());

    SignalSpy playCalledSpy(&mockService, SIGNAL(mock_playCalled(QString,QVariantMap)));
    SignalSpy eventPlayingSpy(m_client, SIGNAL(eventPlaying(quint32)));
    SignalSpy eventFailedSpy(m_client, SIGNAL(eventFailed(quint32)));

    QVariantMap properties;
    properties["foo"] = "fooval";
    properties["bar"] = 42;

    mockService.call("mock_failNextPlay");

    quint32 id = m_client->play("an-event", properties);
    QVERIFY(id > 0);

    // Note: while we wait for mock_playCalled signal, a reply to Play() is received by Client
    // internally, and this is crucial for the other test cases to success as the server_event_id is
    // not known before Play() returns (called asynchronously)

    QVERIFY(waitForSignals(SignalSpyList() << &playCalledSpy << &eventFailedSpy));
    QCOMPARE(playCalledSpy.count(), 1);
    QCOMPARE(playCalledSpy.at(0).at(0).toString(), QString("an-event"));
    QCOMPARE(playCalledSpy.at(0).at(1).toMap(), properties);
    QCOMPARE(eventFailedSpy.count(), 1);
    QCOMPARE(eventFailedSpy.at(0).at(0).toUInt(), 3u);
    QCOMPARE(eventPlayingSpy.count(), 0);
}

void UtClient::testConnectionStatus()
{
    QSKIP("Libngf-qt not currently tracking the daemon availability");
    QDBusInterface mockService(service(), path(), interface(), bus());

    SignalSpy connectionStatusSpy(m_client, SIGNAL(connectionStatus(bool)));

    mockService.call("mock_disconnectForAWhile");

    QList<bool> expected = QList<bool>() << false << true;
    while (!expected.isEmpty()) {
        QVERIFY(waitForSignal(&connectionStatusSpy));
        while (!connectionStatusSpy.isEmpty()) {
            QCOMPARE(connectionStatusSpy.at(0).at(0).toBool(), expected.at(0));
            connectionStatusSpy.removeAt(0);
            expected.removeAt(0);
        }
    }
}

void UtClient::testFastPlayStop()
{
    QDBusInterface mockService(service(), path(), interface(), bus());

    SignalSpy eventCompletedSpy(m_client, SIGNAL(eventCompleted(quint32)));

    QVariantMap properties;
    properties["foo"] = "fooval";
    properties["bar"] = 42;

    quint32 id = m_client->play("an-event", properties);
    QVERIFY(id > 0);
    QVERIFY(m_client->stop("an-event"));

    QVERIFY(waitForSignal(&eventCompletedSpy));
    QCOMPARE(eventCompletedSpy.count(), 1);
    QCOMPARE(eventCompletedSpy.at(0).at(0).toUInt(), 4u);
}

TEST_MAIN(UtClient)

#include "ut_client.moc"

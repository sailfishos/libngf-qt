#ifndef TESTBASE_H
#define TESTBASE_H

#include <QtCore/QObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusServiceWatcher>
#include <QtTest/QSignalSpy>
#include <QTest>
#include <QSet>

#define QTOSTRING_HELPER(s) #s
#define QTOSTRING(s) QTOSTRING_HELPER(s)

Q_DECLARE_METATYPE(QDBusPendingCallWatcher*) // needed by waitForSignal

namespace Ngf {
namespace Tests {

class TestBase : public QObject
{
public:
    class NgfdMock;

protected:
    class SignalSpy;
    typedef QList<SignalSpy *> SignalSpyList;

    enum {
        SIGNAL_WAIT_TIMEOUT = 5000, // [ms]
    };

protected:
    TestBase();

protected:
    static QDBusConnection bus() { return QDBusConnection::systemBus(); }
    static QString service() { return "com.nokia.NonGraphicFeedback1.Backend"; }
    static QString path() { return "/com/nokia/NonGraphicFeedback1"; }
    static QString interface() { return "com.nokia.NonGraphicFeedback1"; }
    static QByteArray notifySignal(const QObject &object, const char *property);
    static bool waitForSignal(QObject *object, const char *signal);
    static bool waitForSignal(SignalSpy *signalSpy);
    static bool waitForSignals(const SignalSpyList &signalSpies);
    static bool waitForService(const QString &serviceName);
    static void createTestPropertyData(const QVariantMap &expected,
            QString (*dbusProperty2QtProperty)(const QString &));
    static void testProperty(const QObject &object, const QString &property,
        const QVariant &expected);
    static void testWriteProperty(QObject *object, const char *property,
        const QVariant &newValue);
    static void testWriteProperty(QObject *object, QObject *otherObject, const char *property,
        const QVariant &newValue);
    static QString clientDBusProperty2QtProperty(const QString &property);
    static QVariantMap defaultClientProperties();
    static QVariantMap alternateClientProperties();
};

class TestBase::NgfdMock : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.nokia.NonGraphicFeedback1")

    enum NgfStatusId
    {
        StatusEventFailed       = 0,
        StatusEventCompleted    = 1,
        StatusEventPlaying      = 2,
        StatusEventPaused       = 3,
    };

public:
    NgfdMock();

public:
    Q_SCRIPTABLE quint32 Play(const QString &event, const QVariantMap &properties,
            const QDBusMessage &message);
    Q_SCRIPTABLE void Pause(quint32 event, bool pause, const QDBusMessage &message);
    Q_SCRIPTABLE void Stop(quint32 event, const QDBusMessage &message);

    // mock API
    Q_SCRIPTABLE quint32 mock_id(const QString &event) const;
    Q_SCRIPTABLE QVariantMap mock_properties(const QString &event) const;
    Q_SCRIPTABLE bool mock_isPaused(const QString &event) const;
    Q_SCRIPTABLE void mock_stop(const QString &event, const QDBusMessage &message);
    Q_SCRIPTABLE void mock_fail(const QString &event, const QDBusMessage &message);
    Q_SCRIPTABLE void mock_failNextPlay();
    Q_SCRIPTABLE void mock_disconnectForAWhile(const QDBusMessage &message);

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
    static void installMsgHandler();

signals:
    Q_SCRIPTABLE void Status(quint32 event, quint32 status);

    // mock API
    Q_SCRIPTABLE void mock_playCalled(const QString &event, const QVariantMap &properties);
    Q_SCRIPTABLE void mock_pauseCalled(quint32 event, bool pause);
    Q_SCRIPTABLE void mock_stopCalled(quint32 event);

private:
    int m_maxId;
    bool m_failNextPlay;
    QMap<QString, QPair<quint32, QVariantMap> > m_events;
    QMap<quint32, QString> m_eventId2Name;
    QSet<QString> m_paused;
};

/*
 * QSignalSpy is inherited to fix two issues when used together with QDBusInterface:
 *
 * 1. QDBusInterface relies on QObject::connectNotify() to connect the D-Bus signal immediately when
 *    QObject::connect() is called. QSignalSpy uses internal version of QObject::connect() which
 *    does not call connectNotify(). This issue is fixed as a side effect of the fix to the second
 *    issue -- otherwise it would be necessary to invoke connectNotify() explicitely upon SignalSpy
 *    construction.
 * 2. TestBase::waitForSignals() uses combination of SignalSpy and QEventLoop to wait for signals.
 *    It connects the signals to QEventLoop::quit(). When used together with QDBusInterface, it is
 *    usually too late to connect -- that is why the signalEmitted() signal exists.
 */
class TestBase::SignalSpy : public QObject, public QSignalSpy
{
    Q_OBJECT

public:
    SignalSpy(QObject *object, const char *signal)
        : QObject()
        , QSignalSpy(object, signal)
    {
        connect(object, signal, this, SIGNAL(signalEmitted()));
    }

signals:
    void signalEmitted();
};

inline TestBase::TestBase()
{
    qRegisterMetaType<QDBusPendingCallWatcher *>(); // needed by waitForSignal
}

inline QByteArray TestBase::notifySignal(const QObject &object, const char *property)
{
    Q_ASSERT(object.metaObject()->indexOfProperty(property) != -1);
    Q_ASSERT(object.metaObject()->property(object.metaObject()->indexOfProperty(property))
            .hasNotifySignal());

    return QByteArray(QTOSTRING(QSIGNAL_CODE)).append(object.metaObject()->property(
                object.metaObject()->indexOfProperty(property)
                ).notifySignal()
                .methodSignature()
                );
}

inline bool TestBase::waitForSignal(QObject *object, const char *signal)
{
    SignalSpy spy(object, signal);
    return waitForSignals(SignalSpyList() << &spy);
}

inline bool TestBase::waitForSignal(SignalSpy *signalSpy)
{
    return waitForSignals(SignalSpyList() << signalSpy);
}

inline bool TestBase::waitForSignals(const SignalSpyList &signalSpies)
{
    struct H
    {
        static bool allReceived(const SignalSpyList &signalSpies)
        {
            foreach (SignalSpy *signalSpy, signalSpies) {
                if (signalSpy->count() == 0) {
                    return false;
                }
            }
            return true;
        }
    };

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    connect(&timeoutTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
    foreach (SignalSpy *signalSpy, signalSpies) {
        connect(signalSpy, SIGNAL(signalEmitted()), &loop, SLOT(quit()));
    }

    timeoutTimer.start(SIGNAL_WAIT_TIMEOUT);

    while (!H::allReceived(signalSpies)) {
        loop.exec();
        if (!timeoutTimer.isActive()) {
            return false;
        }
    }

    return true;
}

inline bool TestBase::waitForService(const QString &serviceName)
{
    if (bus().interface()->registeredServiceNames().value().contains(serviceName)) {
        return true;
    }

    QDBusServiceWatcher watcher(serviceName, bus(), QDBusServiceWatcher::WatchForRegistration);
    SignalSpy spy(&watcher, SIGNAL(serviceRegistered(QString)));

    return waitForSignal(&spy);
}

inline void TestBase::createTestPropertyData(const QVariantMap &expected,
        QString (*dbusProperty2QtProperty)(const QString &))
{
    QMapIterator<QString, QVariant> it(expected);

    while (it.hasNext()) {
        it.next();

        const QString qtProperty = (dbusProperty2QtProperty)(it.key());
        if (qtProperty.isEmpty()) {
            continue;
        }

        if (it.value().canConvert(QVariant::Map)) {
            const QVariantMap mapTypeProperty = it.value().toMap();
            QMapIterator<QString, QVariant> it2(mapTypeProperty);
            while (it2.hasNext()) {
                it2.next();

                QTest::newRow(qPrintable(QString("%1/%2").arg(qtProperty).arg(it2.key())))
                    << it2.value();
            }
        } else {
            QTest::newRow(qPrintable(qtProperty)) << it.value();
        }
    }
}

inline void TestBase::testProperty(const QObject &object, const QString &property,
        const QVariant &expected)
{
    Q_ASSERT(property.count('/') <= 1);

    const QString propertyName = QString(property).section('/', 0, 0);
    const QString variantMapKey = QString(property).section('/', 1, 1);

    QVERIFY(object.property(qPrintable(propertyName)).isValid());

    if (variantMapKey.isEmpty()) {
        QVERIFY(object.property(qPrintable(propertyName)).type() != QVariant::Map);
        QCOMPARE(object.property(qPrintable(propertyName)), expected);
    } else {
        QCOMPARE(object.property(qPrintable(propertyName)).type(), QVariant::Map);
        QVERIFY(object.property(qPrintable(propertyName)).toMap().contains(variantMapKey));
        QCOMPARE(object.property(qPrintable(propertyName)).toMap()[variantMapKey], expected);
    }
}

inline void TestBase::testWriteProperty(QObject *object, const char *property,
        const QVariant &newValue)
{
    Q_ASSERT(!QString(property).contains('/'));

    const QByteArray notifySignal = TestBase::notifySignal(*object, property);

    SignalSpy spy(object, notifySignal);

    QVERIFY(object->setProperty(property, newValue));

    QVERIFY(waitForSignals(SignalSpyList() << &spy));

    // TODO: test signal argument if available
    QVERIFY(object->property(property).isValid());
    QCOMPARE(object->property(property), newValue);
}

inline void TestBase::testWriteProperty(QObject *object, QObject *otherObject, const char *property,
        const QVariant &newValue)
{
    Q_ASSERT(!QString(property).contains('/'));

    const QByteArray notifySignal = TestBase::notifySignal(*object, property);

    SignalSpy spy(object, notifySignal);
    SignalSpy otherSpy(otherObject, notifySignal);

    QVERIFY(object->setProperty(property, newValue));

    QVERIFY(waitForSignals(SignalSpyList() << &spy << &otherSpy));

    // TODO: test signal argument if available
    QVERIFY(object->property(property).isValid());
    QCOMPARE(object->property(property), newValue);
    QVERIFY(otherObject->property(property).isValid());
    QCOMPARE(otherObject->property(property), newValue);
}

inline QString TestBase::clientDBusProperty2QtProperty(const QString &property)
{
    Q_ASSERT(false);
    const QString firstLower = QString(property).replace(0, 1, property.at(0).toLower());
    return firstLower;
}

inline QVariantMap TestBase::defaultClientProperties()
{
    Q_ASSERT(false);
    static QVariantMap properties;
    static bool initialized = false;

    if (initialized) {
        return properties;
    }

    properties["foo"] = "bar";

    initialized = true;

    return properties;
}

inline QVariantMap TestBase::alternateClientProperties()
{
    Q_ASSERT(false);
    static QVariantMap properties;
    static bool initialized = false;

    if (initialized) {
        return properties;
    }

    properties["State"] = "disconnect";

    initialized = true;

    return properties;
}

/*
 * \class Tests::TestBase::NgfdMock
 */

inline TestBase::NgfdMock::NgfdMock()
    : m_maxId(0),
      m_failNextPlay(false)
{
    if (!bus().registerObject(path(), this, QDBusConnection::ExportScriptableContents)) {
        qFatal("Failed to register mock D-Bus object at path '%s': '%s'",
                qPrintable(path()), qPrintable(bus().lastError().message()));
    }

    if (!bus().registerService(service())) {
        qFatal("Failed to register mock D-Bus service '%s': '%s'",
                qPrintable(service()), qPrintable(bus().lastError().message()));
    }
}

inline quint32 TestBase::NgfdMock::Play(const QString &event, const QVariantMap &properties,
        const QDBusMessage &message)
{
    Q_ASSERT(!m_events.contains(event));

    if (m_failNextPlay) {
        m_failNextPlay = false;

        bus().send(message.createErrorReply(QDBusError::InvalidArgs, "mock_failNextPlay-requested"));

        emit mock_playCalled(event, properties);

        return 0;
    }

    const quint32 id = ++m_maxId;

    m_events[event] = qMakePair(id, properties);
    m_eventId2Name[id] = event;

    bus().send(message.createReply(id));

    emit mock_playCalled(event, properties);

    return 0;
}

inline void TestBase::NgfdMock::Pause(quint32 event, bool pause, const QDBusMessage &message)
{
    if (!m_eventId2Name.contains(event)) {
        bus().send(message.createErrorReply(QDBusError::InvalidArgs, "Unknown event"));
        return;
    }

    bus().send(message.createReply());

    if (pause) {
        m_paused.insert(m_eventId2Name.value(event));
        emit Status(event, StatusEventPaused);
    } else {
        m_paused.remove(m_eventId2Name.value(event));
        emit Status(event, StatusEventPlaying);
    }

    emit mock_pauseCalled(event, pause);
}

inline void TestBase::NgfdMock::Stop(quint32 event, const QDBusMessage &message)
{
    if (!m_eventId2Name.contains(event)) {
        bus().send(message.createErrorReply(QDBusError::InvalidArgs, "Unknown event"));
        return;
    }

    bus().send(message.createReply());

    m_paused.remove(m_eventId2Name.value(event));
    m_events.remove(m_eventId2Name.value(event));
    m_eventId2Name.remove(event);

    emit Status(event, StatusEventCompleted);
    emit mock_stopCalled(event);
}

inline void TestBase::NgfdMock::messageHandler(QtMsgType type, const QMessageLogContext &context,
    const QString &message)
{
    qInstallMessageHandler(0);
    qt_message_output(type, context, QString("MOCK   : ").append(message));
    qInstallMessageHandler(messageHandler);
}

inline void TestBase::NgfdMock::installMsgHandler()
{
  qInstallMessageHandler(messageHandler);
}

inline quint32 TestBase::NgfdMock::mock_id(const QString &event) const
{
    return m_events.value(event).first;
}

inline QVariantMap TestBase::NgfdMock::mock_properties(const QString &event) const
{
    return m_events.value(event).second;
}

inline bool TestBase::NgfdMock::mock_isPaused(const QString &event) const
{
    return m_paused.contains(event);
}

inline void TestBase::NgfdMock::mock_stop(const QString &event, const QDBusMessage &message)
{
    if (!m_events.contains(event)) {
        bus().send(message.createErrorReply(QDBusError::InvalidArgs, "Unknown event"));
        return;
    }

    bus().send(message.createReply());

    const quint32 eventId = m_eventId2Name.key(event);

    m_paused.remove(event);
    m_events.remove(event);
    m_eventId2Name.remove(eventId);

    emit Status(eventId, StatusEventCompleted);
}

inline void TestBase::NgfdMock::mock_fail(const QString &event, const QDBusMessage &message)
{
    if (!m_events.contains(event)) {
        bus().send(message.createErrorReply(QDBusError::InvalidArgs, "Unknown event"));
        return;
    }

    bus().send(message.createReply());

    const quint32 eventId = m_eventId2Name.key(event);

    m_paused.remove(event);
    m_events.remove(event);
    m_eventId2Name.remove(eventId);

    emit Status(eventId, StatusEventFailed);
}

inline void TestBase::NgfdMock::mock_failNextPlay()
{
    m_failNextPlay = true;
}

inline void TestBase::NgfdMock::mock_disconnectForAWhile(const QDBusMessage &message)
{
    bus().send(message.createReply());

    if (!bus().unregisterService(service())) {
        qFatal("Failed to unregister mock D-Bus service '%s': '%s'",
            qPrintable(service()), qPrintable(bus().lastError().message()));
    }

    if (!bus().registerService(service())) {
        qFatal("Failed to register mock D-Bus service: '%s' '%s'",
            qPrintable(service()), qPrintable(bus().lastError().message()));
    }
}

#define TEST_MAIN(TestClass)                                                \
    int main(int argc, char *argv[])                                        \
    {                                                                       \
        qputenv("DBUS_SYSTEM_BUS_ADDRESS",                                  \
                qgetenv("DBUS_SESSION_BUS_ADDRESS"));                       \
                                                                            \
        if (argc == 2 && argv[1] == QLatin1String("--mock")) {              \
            Ngf::Tests::TestBase::NgfdMock::installMsgHandler();            \
            QCoreApplication app(argc, argv);                               \
                                                                            \
            qDebug("%s: starting...", Q_FUNC_INFO);                         \
                                                                            \
            Ngf::Tests::TestBase::NgfdMock mock;                            \
                                                                            \
            return app.exec();                                              \
        } else {                                                            \
            QCoreApplication app(argc, argv);                               \
                                                                            \
            TestClass test;                                                 \
                                                                            \
            QProcess mock;                                                  \
            mock.setProcessChannelMode(QProcess::ForwardedChannels);        \
            mock.start(app.applicationFilePath(), QStringList("--mock"));   \
            if (mock.state() == QProcess::NotRunning) {                     \
                qFatal("Failed to start mock");                             \
            }                                                               \
                                                                            \
            const int retv = QTest::qExec(&test, argc, argv);               \
                                                                            \
            mock.terminate();                                               \
            mock.waitForFinished();                                         \
                                                                            \
            return retv;                                                    \
        }                                                                   \
    }                                                                       \


} // namespace Tests
} // namespace Ngf

#endif // TESTBASE_H

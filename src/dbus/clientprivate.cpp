/*
 * NgfClient - Qt Non-Graphic Feedback daemon client library
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: juho.hamalainen@tieto.com
 *
 * This work is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <QObject>
#include <QTimer>
#include <QtDBus>
#include <QList>
#include <QtAlgorithms>
#include "clientprivate.h"

namespace Ngf
{
    enum NgfStatusId
    {
        StatusEventFailed       = 0,
        StatusEventCompleted    = 1,
        StatusEventPlaying      = 2,
        StatusEventPaused       = 3,
    };

    const static QString NgfDestination     = "com.nokia.NonGraphicFeedback1.Backend";
    const static QString NgfPath            = "/com/nokia/NonGraphicFeedback1";
    const static QString NgfInterface       = "com.nokia.NonGraphicFeedback1";
    const static QString MethodPlay         = "Play";
    const static QString MethodStop         = "Stop";
    const static QString MethodPause        = "Pause";
    const static QString SignalStatus       = "Status";

    class Event
    {
    public:
        Event(const QString &_name, const quint32 &_clientEventId, QDBusPendingCallWatcher *_watcher)
            : name(_name), clientEventId(_clientEventId), serverEventId(0),
              wantedState(ClientPrivate::StatePlaying), activeState(ClientPrivate::StateNew),
              watcher(_watcher)
        {}
        ~Event() {}

        QString name;
        quint32 clientEventId;
        quint32 serverEventId;
        ClientPrivate::EventState wantedState;
        ClientPrivate::EventState activeState;
        QDBusPendingCallWatcher *watcher;
    };
};

Ngf::ClientPrivate::ClientPrivate(Client *parent)
    : QObject(parent),
      q_ptr(parent),
      m_log("ngf.client"),
      m_serviceWatcher(0),
      m_connectionWanted(false),
      m_available(false),
      m_connected(false),
      m_iface(0),
      m_clientEventId(0)
{
    m_log.setEnabled(QtDebugMsg, false);
}

Ngf::ClientPrivate::~ClientPrivate()
{
    disconnect();
    removeAllEvents();
}

bool Ngf::ClientPrivate::connect()
{
    m_connectionWanted = true;

    if (m_iface)
        return true;

    if (!m_serviceWatcher) {
        m_serviceWatcher = new QDBusServiceWatcher(NgfDestination,
                                                   QDBusConnection::systemBus(),
                                                   QDBusServiceWatcher::WatchForRegistration |
                                                     QDBusServiceWatcher::WatchForUnregistration,
                                                   this);

        QObject::connect(m_serviceWatcher, SIGNAL(serviceRegistered(const QString&)),
                         this, SLOT(serviceRegistered(const QString&)));
        QObject::connect(m_serviceWatcher, SIGNAL(serviceUnregistered(const QString&)),
                         this, SLOT(serviceUnregistered(const QString&)));

        // When connecting to bus for the first time, check whether NGFD exists in the bus.
        // After we know the initial state it is enough to follow NameOwnerChanged.
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("/org/freedesktop/DBus"),
                                                          QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("GetNameOwner"));
        QList<QVariant> args;
        args << NgfDestination;
        msg.setArguments(args);

        QDBusMessage reply = QDBusConnection::systemBus().call(msg);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            changeConnected(false);
            changeAvailable(false);
            return false;
        } else
            changeAvailable(true);
    }

    if (!m_available) {
        changeConnected(false);
        return false;
    }

    QDBusInterface *iface;
    iface = new QDBusInterface(NgfDestination, NgfPath, NgfInterface,
                               QDBusConnection::systemBus(), this);

    // Match NGFD signals to get statuses for ongoing events
    if (iface) {
        if (iface->isValid()) {
            iface->connection().connect(QStringLiteral(""), NgfPath, NgfInterface, SignalStatus,
                                        this, SLOT(eventStatus(const quint32&, const quint32&)));
            m_iface = iface;
            changeConnected(true);
        } else
            iface->deleteLater();
    }

    return m_connected;
}

void Ngf::ClientPrivate::disconnect()
{
    m_connectionWanted = false;

    if (m_iface) {
        m_iface->deleteLater();
        m_iface = 0;
    }

    changeConnected(false);
}

void Ngf::ClientPrivate::serviceRegistered(const QString &service)
{
    Q_UNUSED(service);

    changeAvailable(true);

    // NGFD has reappeared in system bus, let's reconnect
    if (m_connectionWanted)
        connect();
}

void Ngf::ClientPrivate::serviceUnregistered(const QString &service)
{
    Q_UNUSED(service);

    // NGFD has died for some reason
    changeAvailable(false);
    // All currently active events are invalid, so clear event list
    removeAllEvents();
    if (m_iface) {
        m_iface->deleteLater();
        m_iface = 0;
    }

    // Signal that we are disconnected
    if (m_connectionWanted)
        changeConnected(false);
}

bool Ngf::ClientPrivate::isConnected()
{
    return m_iface;
}

void Ngf::ClientPrivate::eventStatus(const quint32 &serverEventId, const quint32 &state)
{
    Event *event = 0;

    // Look through all ongoing events and match serverEventId to internal clientEventId.
    // In case of failing or completing event, we'll also remove that event from event list later.
    for (int i = 0; i < m_events.size(); ++i) {
        Event *e = m_events.at(i);
        if (e->serverEventId == serverEventId) {
            event = e;
            break;
        }
    }

    if (!event)
        return;

    qCDebug(m_log) << event->clientEventId << "server state" << state;

    switch (state) {
        case StatusEventFailed:
            event->activeState = StateStopped;
            emit q_ptr->eventFailed(event->clientEventId);
            break;

        case StatusEventCompleted:
            event->activeState = StateStopped;
            emit q_ptr->eventCompleted(event->clientEventId);
            break;

        case StatusEventPlaying:
            event->activeState = StatePlaying;
            emit q_ptr->eventPlaying(event->clientEventId);
            break;

        case StatusEventPaused:
            event->activeState = StatePaused;
            emit q_ptr->eventPaused(event->clientEventId);
            break;

        default:
            // Undefined state received from NGFD, probably server
            // DBus API has changed and we are out of sync.
            qCWarning(m_log) << "Client received unknown event state id, likely NGFD API has changed. state:" << state;
            event->activeState = StateStopped;
            emit q_ptr->eventFailed(event->clientEventId);
            removeEvent(event);
            return;
    }

    if (state == StatusEventFailed || state == StatusEventCompleted)
        removeEvent(event);
    else
        setEventState(event, event->wantedState);
}

quint32 Ngf::ClientPrivate::play(const QString &event)
{
    Proplist empty;

    return play(event, empty);
}

quint32 Ngf::ClientPrivate::play(const QString &event, const Proplist &properties)
{
    if (!m_iface)
        return 0;

    ++m_clientEventId;

    // Create asynchronic call to NGFD and connect pending call watcher to slot
    // playPendingReply where it is finally determined if event is really running
    // in the NGFD side.
    QDBusPendingCall pending = m_iface->asyncCall(MethodPlay, event, properties);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, 0);
    Event *e = new Event(event, m_clientEventId, watcher);
    m_events.push_back(e);

    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(playPendingReply(QDBusPendingCallWatcher*)));

    qCDebug(m_log) << e->clientEventId << "set state" << e->wantedState;

    return e->clientEventId;
}

void Ngf::ClientPrivate::playPendingReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<quint32> reply = *watcher;
    int i = 0;

    // Play -method reply should contain one argument of type uint32 containing
    // server side event id for started event.

    for (i = 0; i < m_events.size(); ++i) {
        Event *event = m_events.at(i);
        if (event->watcher == watcher) {
            if (reply.isError() || reply.count() != 1) {
                // Starting event failed for some reason, reason can hopefully be determined from
                // NGFD logs.
                quint32 clientEventId = event->clientEventId;
                removeEvent(event);
                qCDebug(m_log) << clientEventId << "play: operation failed";
                emit q_ptr->eventFailed(clientEventId);
            } else {
                event->serverEventId = reply.argumentAt<0>();
                event->watcher = 0;
                event->activeState = StatePlaying;
                qCDebug(m_log) << event->clientEventId << "play: server replied" << event->serverEventId;
                emit q_ptr->eventPlaying(event->clientEventId);

                if (event->activeState != event->wantedState) {
                    qCDebug(m_log) << event->clientEventId
                                   << "wanted state" << event->wantedState
                                   << "differs from active state" << event->activeState;
                    setEventState(event, event->wantedState);
                }
            }
            break;
        }
    }

    watcher->deleteLater();
}

bool Ngf::ClientPrivate::pause(const quint32 &eventId)
{
    return changeState(eventId, StatePaused);
}

bool Ngf::ClientPrivate::pause(const QString &event)
{
    return changeState(event, StatePaused);
}

bool Ngf::ClientPrivate::resume(const quint32 &eventId)
{
    return changeState(eventId, StatePlaying);
}

bool Ngf::ClientPrivate::resume(const QString &event)
{
    return changeState(event, StatePlaying);
}

bool Ngf::ClientPrivate::stop(const quint32 &eventId)
{
    return changeState(eventId, StateStopped);
}

bool Ngf::ClientPrivate::stop(const QString &event)
{
    return changeState(event, StateStopped);
}

void Ngf::ClientPrivate::removeEvent(Event *event)
{
    if (m_events.removeOne(event))
        delete event;
    else
        qCWarning(m_log) << "Couldn't find event from event list.";
}

void Ngf::ClientPrivate::removeAllEvents()
{
    qDeleteAll(m_events);
    m_events.clear();
}

bool Ngf::ClientPrivate::changeState(const quint32 &clientEventId, EventState wantedState)
{
    if (!m_iface)
        return false;

    for (int i = 0; i < m_events.size(); ++i) {
        Event *e = m_events.at(i);
        if (e->clientEventId == clientEventId) {
            setEventState(e, wantedState);
            return true;
        }
    }

    return false;
}

bool Ngf::ClientPrivate::changeState(const QString &clientEventName, EventState wantedState)
{
    bool result = false;

    if (!m_iface)
        return false;

    for (int i = 0; i < m_events.size(); ++i) {
        Event *e = m_events.at(i);
        if (e->name == clientEventName) {
            setEventState(e, wantedState);
            result = true;
        }
    }

    return result;
}

void Ngf::ClientPrivate::setEventState(Event *event, EventState wantedState)
{
    event->wantedState = wantedState;

    if (event->wantedState == event->activeState ||
        event->activeState == StateStopped ||
        event->activeState == StateNew)
        return;

    qCDebug(m_log) << event->clientEventId << "set state" << event->wantedState;

    switch (event->wantedState) {
        case StatePlaying:
            m_iface->asyncCall(MethodPause, event->serverEventId, QVariant(false));
            break;

        case StatePaused:
            m_iface->asyncCall(MethodPause, event->serverEventId, QVariant(true));
            break;

        case StateStopped:
            m_iface->asyncCall(MethodStop, event->serverEventId);
            break;

        case StateNew:
            break;
    }
}

void Ngf::ClientPrivate::changeAvailable(bool available)
{
    if (m_available != available) {
        m_available = available;
        qCDebug(m_log) << "NGFD available changes to" << m_available;
    }
}

void Ngf::ClientPrivate::changeConnected(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        emit q_ptr->connectionStatus(m_connected);
    }
}

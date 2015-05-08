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
        Event(const QString &_name, const quint32 &_client_event_id, QDBusPendingCallWatcher *_watcher)
            : name(_name), client_event_id(_client_event_id), server_event_id(0),
              wanted_state(ClientPrivate::StatePlaying), active_state(ClientPrivate::StateNew),
              watcher(_watcher)
        {}
        ~Event() {}

        QString name;
        quint32 client_event_id;
        quint32 server_event_id;
        ClientPrivate::EventState wanted_state;
        ClientPrivate::EventState active_state;
        QDBusPendingCallWatcher *watcher;
    };
};

Ngf::ClientPrivate::ClientPrivate(Client *parent)
    : QObject(parent),
      q_ptr(parent),
      m_log("ngf.client"),
      m_connection("ngfclientpp"),
      m_iface(0),
      m_client_event_id(0)
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
    bool new_connection = false;
    bool connection_status = false;

    if (m_iface)
        return true;

    if (!m_connection.isConnected()) {
        m_connection = QDBusConnection::systemBus();
        new_connection = true;
    }

    if (m_connection.isConnected()) {
        // Connect NameOwnerChanged signal only the first time we connect to DBus bus.
        if (new_connection)
            m_connection.connect("", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameOwnerChanged",
                                 this, SLOT(ngfdStatus(const QString&, const QString&, const QString&)));

        QDBusInterface *iface = 0;
        iface = new QDBusInterface(NgfDestination, NgfPath, NgfInterface,
                                   m_connection, 0);

        // Match NGFD signals to get statuses for ongoing events
        if (iface && iface->isValid()) {
            m_connection.connect(QString(), NgfPath, NgfInterface, SignalStatus,
                                 this, SLOT(eventStatus(const quint32&, const quint32&)));

            m_iface = iface;
            connection_status = true;
        } else
            iface->deleteLater();
    }

    emit q_ptr->connectionStatus(connection_status);
    return connection_status;
}

void Ngf::ClientPrivate::disconnect()
{
    if (m_iface) {
        m_iface->deleteLater();
        m_iface = 0;
    }
    m_connection.disconnectFromBus("ngfclientpp");

    m_client_event_id = 0;

    emit q_ptr->connectionStatus(false);
}

// Watch for NameOwnerChanged for NGF daemon, and disconnect and reconnect according to
// NGFd availability
void Ngf::ClientPrivate::ngfdStatus(const QString &service, const QString &arg1, const QString &arg2)
{
    if (service == NgfDestination) {
        if (arg1.size() == 0 && arg2.size() > 0) {
            // NGFD has reappeared in system bus, let's reconnect
            connect();
        } else if (arg1.size() > 0 && arg2.size() == 0) {
            // NGFD has died for some reason
            QDBusInterface *iface = m_iface;
            m_iface = 0;
            // All currently active events are invalid, so clear event list
            removeAllEvents();
            if (iface) {
                // Signal that we are disconnected
                emit q_ptr->connectionStatus(false);
                iface->deleteLater();
            }
        }
    }
}

bool Ngf::ClientPrivate::isConnected()
{
    return (m_connection.isConnected() && m_iface);
}

void Ngf::ClientPrivate::eventStatus(const quint32 &server_event_id, const quint32 &state)
{
    Event *event = 0;

    // Look through all ongoing events and match server_event_id to internal client_event_id.
    // In case of failing or completing event, we'll also remove that event from event list later.
    for (int i = 0; i < m_events.size(); ++i) {
        Event *e = m_events.at(i);
        if (e->server_event_id == server_event_id) {
            event = e;
            break;
        }
    }

    if (!event)
        return;

    qCDebug(m_log) << event->client_event_id << "server state" << state;

    switch (state) {
        case StatusEventFailed:
            event->active_state = StateStopped;
            emit q_ptr->eventFailed(event->client_event_id);
            break;

        case StatusEventCompleted:
            event->active_state = StateStopped;
            emit q_ptr->eventCompleted(event->client_event_id);
            break;

        case StatusEventPlaying:
            event->active_state = StatePlaying;
            emit q_ptr->eventPlaying(event->client_event_id);
            break;

        case StatusEventPaused:
            event->active_state = StatePaused;
            emit q_ptr->eventPaused(event->client_event_id);
            break;

        default:
            // Undefined state received from NGFD, probably server
            // DBus API has changed and we are out of sync.
            qCWarning(m_log) << "Client received unknown event state id, likely NGFD API has changed. state:" << state;
            event->active_state = StateStopped;
            emit q_ptr->eventFailed(event->client_event_id);
            removeEvent(event);
            return;
    }

    if (state == StatusEventFailed || state == StatusEventCompleted)
        removeEvent(event);
    else
        setEventState(event, event->wanted_state);
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

    ++m_client_event_id;

    // Create asynchronic call to NGFD and connect pending call watcher to slot
    // playPendingReply where it is finally determined if event is really running
    // in the NGFD side.
    QDBusPendingCall pending = m_iface->asyncCall(MethodPlay, event, properties);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, 0);
    Event *e = new Event(event, m_client_event_id, watcher);
    m_events.push_back(e);

    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(playPendingReply(QDBusPendingCallWatcher*)));

    qCDebug(m_log) << e->client_event_id << "set state" << e->wanted_state;

    return e->client_event_id;
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
                quint32 client_event_id = event->client_event_id;
                removeEvent(event);
                qCDebug(m_log) << client_event_id << "play: operation failed";
                emit q_ptr->eventFailed(client_event_id);
            } else {
                event->server_event_id = reply.argumentAt<0>();
                event->watcher = 0;
                event->active_state = StatePlaying;
                qCDebug(m_log) << event->client_event_id << "play: server replied" << event->server_event_id;
                emit q_ptr->eventPlaying(event->client_event_id);

                if (event->active_state != event->wanted_state) {
                    qCDebug(m_log) << event->client_event_id
                                   << "wanted state" << event->wanted_state
                                   << "differs from active state" << event->active_state;
                    setEventState(event, event->wanted_state);
                }
            }
            break;
        }
    }

    watcher->deleteLater();
}

bool Ngf::ClientPrivate::pause(const quint32 &event_id)
{
    return changeState(event_id, StatePaused);
}

bool Ngf::ClientPrivate::pause(const QString &event)
{
    return changeState(event, StatePaused);
}

bool Ngf::ClientPrivate::resume(const quint32 &event_id)
{
    return changeState(event_id, StatePlaying);
}

bool Ngf::ClientPrivate::resume(const QString &event)
{
    return changeState(event, StatePlaying);
}

bool Ngf::ClientPrivate::stop(const quint32 &event_id)
{
    return changeState(event_id, StateStopped);
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

bool Ngf::ClientPrivate::changeState(const quint32 &client_event_id, EventState wanted_state)
{
    if (!m_iface)
        return false;

    for (int i = 0; i < m_events.size(); ++i) {
        Event *e = m_events.at(i);
        if (e->client_event_id == client_event_id) {
            setEventState(e, wanted_state);
            return true;
        }
    }

    return false;
}

bool Ngf::ClientPrivate::changeState(const QString &client_event_name, EventState wanted_state)
{
    bool result = false;

    if (!m_iface)
        return false;

    for (int i = 0; i < m_events.size(); ++i) {
        Event *e = m_events.at(i);
        if (e->name == client_event_name) {
            setEventState(e, wanted_state);
            result = true;
        }
    }

    return result;
}

void Ngf::ClientPrivate::setEventState(Event *event, EventState wanted_state)
{
    event->wanted_state = wanted_state;

    if (event->wanted_state == event->active_state ||
        event->active_state == StateStopped ||
        event->active_state == StateNew)
        return;

    qCDebug(m_log) << event->client_event_id << "set state" << event->wanted_state;

    switch (event->wanted_state) {
        case StatePlaying:
            m_iface->asyncCall(MethodPause, event->server_event_id, QVariant(false));
            break;

        case StatePaused:
            m_iface->asyncCall(MethodPause, event->server_event_id, QVariant(true));
            break;

        case StateStopped:
            m_iface->asyncCall(MethodStop, event->server_event_id);
            break;

        case StateNew:
            break;
    }
}

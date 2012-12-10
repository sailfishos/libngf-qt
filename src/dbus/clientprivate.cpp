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
#include "clientprivate.h"

#include <QDebug>

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

    struct Event
    {
        Event(const QString &_name, const quint32 &_client_event_id, QDBusPendingCallWatcher *_watcher)
            : name(_name), client_event_id(_client_event_id), server_event_id(0),
              stopped(false), watcher(_watcher)
        {}

        QString name;
        quint32 client_event_id;
        quint32 server_event_id;
        bool stopped;
        QDBusPendingCallWatcher *watcher;
    };
};

Ngf::ClientPrivate::ClientPrivate(Client *parent)
    : QObject(parent),
      q_ptr(parent),
      m_connection("ngfclientpp"),
      m_iface(0),
      m_client_event_id(0)
{}

Ngf::ClientPrivate::~ClientPrivate()
{
    disconnect();
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
            m_events.clear();
            // Signal that we are disconnected
            emit q_ptr->connectionStatus(false);
            if (iface)
                iface->deleteLater();
        }
    }
}

bool Ngf::ClientPrivate::isConnected()
{
    return (m_connection.isConnected() && m_iface);
}

void Ngf::ClientPrivate::eventStatus(const quint32 &server_event_id, const quint32 &state)
{
    quint32 client_event_id = 0;
    QString name;

    // Look through all ongoing events and match server_event_id to internal client_event_id.
    // In case of failing or completing event, we'll also remove that event from event list.
    for (int i = 0; i < m_events.size(); ++i) {
        if (m_events.at(i).server_event_id == server_event_id) {
            client_event_id = m_events.at(i).client_event_id;
            name = m_events.at(i).name;
            if (state == StatusEventFailed || state == StatusEventCompleted)
                removeAt(i);
            break;
        }
    }

    // Signal new event status to client user
    if (client_event_id > 0) {
        switch (state) {
            case StatusEventFailed:
                emit q_ptr->eventFailed(client_event_id);
                break;
            case StatusEventCompleted:
                emit q_ptr->eventCompleted(client_event_id);
                break;
            case StatusEventPlaying:
                emit q_ptr->eventPlaying(client_event_id);
                break;
            case StatusEventPaused:
                emit q_ptr->eventPaused(client_event_id);
                break;
            default:
                // Undefined state received from NGFD, probably server
                // DBus API has changed and we are out of sync.
                qWarning() << "Client received unknown event state id, likely NGFD API has changed. state:" << state;
                break;
        }
    }

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
    Event e(event, m_client_event_id, watcher);
    m_events.push_back(e);

    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(playPendingReply(QDBusPendingCallWatcher*)));

    return e.client_event_id;
}

void Ngf::ClientPrivate::playPendingReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<quint32> reply = *watcher;
    int i = 0;

    // Play -method reply should contain one argument of type uint32 containing
    // server side event id for started event.

    for (i = 0; i < m_events.size(); ++i) {
        if (m_events.at(i).watcher == watcher) {
            if (reply.isError() || reply.count() != 1) {
                // Starting event failed for some reason, reason can hopefully be determined from
                // NGFD logs. Remove event from event list before signalling client user to
                // make sure that event doesn't exist anymore when client user gets the signal.
                quint32 client_event_id = m_events.at(i).client_event_id;
                removeAt(i);
                emit q_ptr->eventFailed(client_event_id);
            } else {
                quint32 server_event_id = reply.argumentAt<0>();
                m_events[i].server_event_id = server_event_id;
                m_events[i].watcher = 0;
            }
        }
    }

    watcher->deleteLater();
}

bool Ngf::ClientPrivate::pause(const quint32 &event_id)
{
    QVariant p(true);
    return doCall(MethodPause, event_id, &p);
}

bool Ngf::ClientPrivate::pause(const QString &event)
{
    QVariant p(true);
    return doCall(MethodPause, event, &p);
}

bool Ngf::ClientPrivate::resume(const quint32 &event_id)
{
    QVariant p(false);
    return doCall(MethodPause, event_id, &p);
}

bool Ngf::ClientPrivate::resume(const QString &event)
{
    QVariant p(false);
    return doCall(MethodPause, event, &p);
}

bool Ngf::ClientPrivate::stop(const quint32 &event_id)
{
    return doCall(MethodStop, event_id, 0);
}

bool Ngf::ClientPrivate::stop(const QString &event)
{
    return doCall(MethodStop, event, 0);
}

void Ngf::ClientPrivate::removeAt(const int &i)
{
    m_events[i].stopped = true;
    m_events.removeAt(i);
}

bool Ngf::ClientPrivate::doCall(const QString &event, const quint32 &client_event_id, const QVariant *extra_arg)
{
    bool result = false;

    if (!m_iface)
        return false;

    // Call NGFD with event and server_event_id, NGFD replies to all successfull event calls with the same
    // server_event_id that it received, or error if there was something wrong with the DBus message.
    // We are confident that the server side API matches our knowledge and won't bother with the reply.
    for (int i = 0; i < m_events.size(); ++i) {
        if (m_events.at(i).client_event_id == client_event_id && !m_events.at(i).stopped) {
            if (extra_arg)
                m_iface->asyncCall(event, m_events.at(i).server_event_id, *extra_arg);
            else
                m_iface->asyncCall(event, m_events.at(i).server_event_id);
            result = true;
        }
    }

    return result;
}

bool Ngf::ClientPrivate::doCall(const QString &event, const QString &client_event_name, const QVariant *extra_arg)
{
    QList<quint32> eventIdList;

    if (!m_iface)
        return false;

    // Search all events that match with given client_event_name and do calls in another loop
    for (int i = 0; i < m_events.size(); ++i) {
        if (m_events.at(i).name == client_event_name && !m_events.at(i).stopped)
            eventIdList.push_back(m_events.at(i).server_event_id);
    }

    // Call NGFD with event and server_event_id, NGFD replies to all successfull event calls with the same
    // server_event_id that it received, or error if there was something wrong with the DBus message.
    // We are confident that the server side API matches our knowledge and won't bother with the reply.
    if (eventIdList.size() > 0) {
        for (int i = 0; i < eventIdList.size(); ++i)
            if (extra_arg)
                m_iface->asyncCall(event, eventIdList.at(i), *extra_arg);
            else
                m_iface->asyncCall(event, eventIdList.at(i));
        return true;
    }

    return false;
}

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

#ifndef NGFCLIENTDBUSPRIVATE_H
#define NGFCLIENTDBUSPRIVATE_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QList>
#include <QLoggingCategory>
#include "ngfclient.h"

namespace Ngf
{
    class Event;

    typedef QMap<QString, QVariant> Proplist;

    class ClientPrivate : public QObject
    {
        Q_OBJECT

    public:
        ClientPrivate(Client *parent);
        virtual ~ClientPrivate();

        bool connect();
        bool isConnected();
        void disconnect();
        quint32 play(const QString &event);
        quint32 play(const QString &event, const Proplist &properties);
        bool pause(const quint32 &event_id);
        bool pause(const QString &event);
        bool resume(const quint32 &event_id);
        bool resume(const QString &event);
        bool stop(const quint32 &event_id);
        bool stop(const QString &event);

        enum EventState {
            StateNew,
            StatePlaying,
            StatePaused,
            StateStopped
        };

    private slots:
        void playPendingReply(QDBusPendingCallWatcher *watcher);
        void eventStatus(const quint32 &server_event_id, const quint32 &state);
        void serviceRegistered(const QString &service);
        void serviceUnregistered(const QString &service);

    private:
        void setEventState(Event *event, EventState wanted_state);
        void removeEvent(Event *event);
        void removeAllEvents();
        bool changeState(const quint32 &client_event_id, EventState wanted_state);
        bool changeState(const QString &client_event_name, EventState wanted_state);
        void changeAvailable(bool available);
        void changeConnected(bool connected);

        Client * const q_ptr;
        Q_DECLARE_PUBLIC(Client);

        QLoggingCategory m_log;
        QDBusServiceWatcher *m_serviceWatcher;
        bool m_connectionWanted;
        bool m_available;
        bool m_connected;
        QDBusInterface *m_iface;
        quint32 m_client_event_id; // Internal counter for client event ids, incremented every time play is called.
        QList<Event*> m_events;
    };

};

#endif

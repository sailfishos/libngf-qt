/* Copyright (C) 2013 Jolla Ltd.
 * Contact: John Brooks <john.brooks@jollamobile.com>
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

#include "declarativengfevent.h"
#include <NgfClient>

static QSharedPointer<Ngf::Client> clientInstance()
{
    static QWeakPointer<Ngf::Client> client;

    QSharedPointer<Ngf::Client> re = client.toStrongRef();
    if (re.isNull()) {
        re = QSharedPointer<Ngf::Client>(new Ngf::Client);
        client = re.toWeakRef();
    }

    return re;
}

DeclarativeNgfEvent::DeclarativeNgfEvent(QObject *parent)
    : QObject(parent), client(clientInstance()), m_status(Stopped), m_eventId(0), m_autostart(false)
{
    connect(client.data(), SIGNAL(connectionStatus(bool)), SLOT(connectionStatusChanged(bool)));
    connect(client.data(), SIGNAL(eventFailed(quint32)), SLOT(eventFailed(quint32)));
    connect(client.data(), SIGNAL(eventCompleted(quint32)), SLOT(eventCompleted(quint32)));
    connect(client.data(), SIGNAL(eventPlaying(quint32)), SLOT(eventPlaying(quint32)));
    connect(client.data(), SIGNAL(eventPaused(quint32)), SLOT(eventPaused(quint32)));
}

DeclarativeNgfEvent::~DeclarativeNgfEvent()
{
    stop();
}

void DeclarativeNgfEvent::setEvent(const QString &event)
{
    if (m_event == event)
        return;

    if (m_eventId) {
        stop();
        m_autostart = true;
    }

    m_event = event;

    emit eventChanged();
    if (m_autostart)
        play();
}

void DeclarativeNgfEvent::play()
{
    if (!isConnected())
        client->connect();

    m_autostart = true;

    if (m_eventId)
        stop();

    if (!m_event.isEmpty() && isConnected())
        m_eventId = client->play(m_event);
}

void DeclarativeNgfEvent::pause()
{
    if (!m_eventId)
        return;

    client->pause(m_eventId);
}

void DeclarativeNgfEvent::resume()
{
    if (!m_eventId)
        return;

    client->resume(m_eventId);
}

void DeclarativeNgfEvent::stop()
{
    m_autostart = false;

    if (!m_eventId)
        return;

    client->stop(m_eventId);
    m_eventId = 0;
    m_status = Stopped;
    emit statusChanged();
}

bool DeclarativeNgfEvent::isConnected() const
{
    return client->isConnected();
}

void DeclarativeNgfEvent::connectionStatusChanged(bool connected)
{
    if (connected && m_autostart) {
        m_autostart = false;
        play();
    }

    emit connectedChanged();
}

void DeclarativeNgfEvent::eventFailed(quint32 id)
{
    if (id != m_eventId)
        return;

    m_eventId = 0;
    m_status = Failed;
    emit statusChanged();
}

void DeclarativeNgfEvent::eventCompleted(quint32 id)
{
    if (id != m_eventId)
        return;

    m_eventId = 0;
    m_status = Stopped;
    emit statusChanged();
}

void DeclarativeNgfEvent::eventPlaying(quint32 id)
{
    if (id != m_eventId)
        return;

    m_status = Playing;
    m_autostart = false;
    emit statusChanged();
}

void DeclarativeNgfEvent::eventPaused(quint32 id)
{
    if (id != m_eventId)
        return;

    m_status = Paused;
    emit statusChanged();
}


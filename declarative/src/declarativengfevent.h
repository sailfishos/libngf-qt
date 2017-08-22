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

#ifndef DECLARATIVENGFEVENT_H
#define DECLARATIVENGFEVENT_H

#include <QObject>
#include <QSharedPointer>

namespace Ngf {
    class Client;
}

class DeclarativeNgfEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString event READ event WRITE setEvent NOTIFY eventChanged)
    Q_PROPERTY(EventStatus status READ status NOTIFY statusChanged)
    Q_ENUMS(EventStatus)

public:
    enum EventStatus {
        Stopped,
        Failed,
        Playing,
        Paused
    };

    DeclarativeNgfEvent(QObject *parent = 0);
    virtual ~DeclarativeNgfEvent();

    bool isConnected() const;

    QString event() const { return m_event; }
    void setEvent(const QString &event);

    EventStatus status() const { return m_status; }

public slots:
    /*!
       \qmlmethod void NonGraphicalFeedback::play()

       Begins playing the defined event. If already playing, playback will be
       restarted from the beginning.

       Actual playback happens asynchronously. The \c status property will change
       when playback begins and ends, or in case of failure.
     */
    void play();
    /*!
       \qmlmethod void NonGraphicalFeedback::pause()

       Pause the currently playing event. Playback can be resumed with \a resume()
     */
    void pause();
    /*!
       \qmlmethod void NonGraphicalFeedback::resume()

       Resume a paused event.
     */
    void resume();
    /*!
       \qmlmethod void NonGraphicalFeedback::stop()

       Stop playback of the event.
     */
    void stop();

signals:
    void connectedChanged();
    void eventChanged();
    void statusChanged();

private slots:
    void connectionStatusChanged(bool connected);
    void eventFailed(quint32 id);
    void eventCompleted(quint32 id);
    void eventPlaying(quint32 id);
    void eventPaused(quint32 id);

private:
    QSharedPointer<Ngf::Client> client;
    QString m_event;
    EventStatus m_status;
    quint32 m_eventId;
    bool m_autostart;
};

#endif


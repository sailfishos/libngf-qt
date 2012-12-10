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
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QMap>
#include <QTimer>

#include <NgfClient>
class Testing : public QObject
{
    Q_OBJECT
public:
    Testing() : c()
    {
        QObject::connect(&c, SIGNAL(connectionStatus(bool)),
                         this, SLOT(connected(bool)));
        QObject::connect(&c, SIGNAL(eventPlaying(const quint32&)),
                         this, SLOT(statusPlay(const quint32&)));
        QObject::connect(&c, SIGNAL(eventFailed(const quint32&)),
                         this, SLOT(statusFail(const quint32&)));
        QObject::connect(&c, SIGNAL(eventCompleted(const quint32&)),
                         this, SLOT(statusComplete(const quint32&)));
        QObject::connect(&c, SIGNAL(eventPaused(const quint32&)),
                         this, SLOT(statusPause(const quint32&)));

        c.connect();
    }

    ~Testing() {};

    Ngf::Client c;

private slots:
    void connected(bool really)
    {
        if (really)
            qDebug() << "Connected to NGFD";
        else {
            qDebug() << "Disconnected from NGFD";
            return;
        }

        QTimer::singleShot(0, this, SLOT(playSMS()));

        QTimer::singleShot(2000, this, SLOT(playRingtone()));
        QTimer::singleShot(6000, this, SLOT(stopRingtone()));

        QTimer::singleShot(8000, this, SLOT(playRingtone()));
        QTimer::singleShot(11000, this, SLOT(pauseRingtone()));
        QTimer::singleShot(13000, this, SLOT(resumeRingtone()));
        QTimer::singleShot(15000, this, SLOT(stopRingtone()));
    }

    void playSMS()
    {
        quint32 id;
        QMap<QString, QVariant> props;
        props.insert(QString("media.audio"), QVariant(true));
        id = c.play("sms", props);
        qDebug() << "CLIENT: PLAY sms" << id;
    }

    void playRingtone()
    {
        quint32 id;
        QMap<QString, QVariant> props;
        props.insert(QString("media.audio"), QVariant(true));
        id = c.play("ringtone", props);
        qDebug() << "CLIENT: PLAY ringtone" << id;
    }

    void pauseRingtone()
    {
        qDebug() << "CLIENT: PAUSE ringtone";
        c.pause("ringtone");
    }


    void resumeRingtone()
    {
        qDebug() << "CLIENT: RESUME ringtone";
        c.resume("ringtone");
    }

    void stopRingtone()
    {
        qDebug() << "CLIENT: stop ringtone";
        c.stop("ringtone");
    }

    void statusPlay(const quint32 &id)
    {
        qDebug() << "CLIENT: STATUS play" << id;
    }

    void statusFail(const quint32 &id)
    {
        qDebug() << "CLIENT: STATUS fail" << id;
    }

    void statusComplete(const quint32 &id)
    {
        qDebug() << "CLIENT: STATUS complete" << id;
    }

    void statusPause(const quint32 &id)
    {
        qDebug() << "CLIENT: STATUS pause" << id;
    }
};


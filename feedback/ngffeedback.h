/*
 * Copyright (c) 2021 Jolla Ltd.
 *
 * This file is part of the libngf QtFeedback plugin.
 * Based on qt5-feedback-hapticks-droid-vibrator.
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

#ifndef NGF_FEEDBACK_H
#define NGF_FEEDBACK_H

#include <QObject>
#include <QLoggingCategory>
#include <qfeedbackplugininterfaces.h>
#include "ngfclient.h"

class NGFFeedback : public QObject, public QFeedbackHapticsInterface, public QFeedbackThemeInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtFeedbackPlugin" FILE "libngf.json")
    Q_INTERFACES(QFeedbackHapticsInterface)
    Q_INTERFACES(QFeedbackThemeInterface)

public:
    NGFFeedback(QObject *parent = 0);
    ~NGFFeedback();

    virtual bool play(QFeedbackEffect::Effect) override;
    virtual QFeedbackInterface::PluginPriority pluginPriority() override;

    virtual QList<QFeedbackActuator*> actuators() override;
    virtual void setActuatorProperty(const QFeedbackActuator &, ActuatorProperty, const QVariant &) override;
    virtual QVariant actuatorProperty(const QFeedbackActuator &, ActuatorProperty) override;
    virtual bool isActuatorCapabilitySupported(const QFeedbackActuator &, QFeedbackActuator::Capability) override;
    virtual void updateEffectProperty(const QFeedbackHapticsEffect *, EffectProperty) override;
    virtual void setEffectState(const QFeedbackHapticsEffect *, QFeedbackEffect::State) override;
    virtual QFeedbackEffect::State effectState(const QFeedbackHapticsEffect *) override;

private slots:
    void failed(quint32 id);
    void completed(quint32 id);
    void playing(quint32 id);
    void paused(quint32 id);

private:
    struct ActiveEffect {
        quint32 id;
        QFeedbackEffect::State state;
        QFeedbackHapticsEffect *effect;

        bool operator==(const ActiveEffect &other) const {
            return id == other.id && state == other.state && effect == other.effect;
        }
    };

    ActiveEffect *findEffect(quint32 id);
    ActiveEffect *findCustomEffect(const QFeedbackHapticsEffect *effect);

    void startCustomEffect(ActiveEffect *active, const QFeedbackHapticsEffect *effect);
    void stopCustomEffect(ActiveEffect *active);
    void pauseCustomEffect(ActiveEffect *active);
    void resumeCustomEffect(ActiveEffect *active);

    QFeedbackActuator *m_actuator;
    bool m_actuatorEnabled;
    QVector<ActiveEffect> m_activeEffects;

    Ngf::Client m_client;
    QString m_effects[QFeedbackEffect::NumberOfEffects];
};

#endif // NGF_FEEDBACK_H

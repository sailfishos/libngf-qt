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

#include <qfeedbackeffect.h>
#include <QCoreApplication>
#include <QtPlugin>
#include "ngffeedback.h"

Q_LOGGING_CATEGORY(ngflc, "qt.Feedback.ngf", QtWarningMsg)

NGFFeedback::NGFFeedback(QObject *parent)
    : QObject(parent)
    , QFeedbackHapticsInterface()
    , QFeedbackThemeInterface()
    , m_actuator(createFeedbackActuator(this, 2))
    , m_actuatorEnabled(true)
    , m_client(this)
{
    qCDebug(ngflc) << "Initializing plugin";

    if (!m_client.connect()) {
        qCCritical(ngflc) << "Unable to connect to NGFD";
    }

    connect(&m_client, &Ngf::Client::eventFailed,
            this, &NGFFeedback::failed);
    connect(&m_client, &Ngf::Client::eventCompleted,
            this, &NGFFeedback::completed);
    connect(&m_client, &Ngf::Client::eventPlaying,
            this, &NGFFeedback::playing);
    connect(&m_client, &Ngf::Client::eventPaused,
            this, &NGFFeedback::paused);

    m_effects[QFeedbackEffect::Press] = QStringLiteral("feedback_press");
    m_effects[QFeedbackEffect::Release] = QStringLiteral("feedback_release");
    m_effects[QFeedbackEffect::PressWeak] = QStringLiteral("feedback_press_weak");
    m_effects[QFeedbackEffect::ReleaseWeak] = QStringLiteral("feedback_release_weak");
    m_effects[QFeedbackEffect::PressStrong] = QStringLiteral("feedback_press_strong");
    m_effects[QFeedbackEffect::ReleaseStrong] = QStringLiteral("feedback_release_strong");
    m_effects[QFeedbackEffect::DragStart] = QStringLiteral("feedback_drag_start");
    m_effects[QFeedbackEffect::DragDropInZone] = QStringLiteral("feedback_drag_drop_in_zone");
    m_effects[QFeedbackEffect::DragDropOutOfZone] = QStringLiteral("feedback_drag_drop_out_of_zone");
    m_effects[QFeedbackEffect::DragCrossBoundary] = QStringLiteral("feedback_drag_cross_boundary");
    m_effects[QFeedbackEffect::Appear] = QString();
    m_effects[QFeedbackEffect::Disappear] = QString();
    m_effects[QFeedbackEffect::Move] = QString();
}

NGFFeedback::~NGFFeedback()
{
    qCDebug(ngflc) << "Deinitializing plugin";
}

void NGFFeedback::failed(quint32 id)
{
    ActiveEffect *active = findEffect(id);
    if (active)
        m_activeEffects.removeAll(*active);
    qCWarning(ngflc) << "Effect failed, id" << id;
}

void NGFFeedback::completed(quint32 id)
{
    ActiveEffect *active = findEffect(id);
    if (active)
        m_activeEffects.removeAll(*active);
    qCDebug(ngflc) << "Effect completed, id" << id;
}

void NGFFeedback::playing(quint32 id)
{
    ActiveEffect *active = findEffect(id);
    if (active)
        active->state = QFeedbackEffect::Running;
    qCDebug(ngflc) << "Effect playing, id" << id;
}

void NGFFeedback::paused(quint32 id)
{
    ActiveEffect *active = findEffect(id);
    if (active)
        active->state = QFeedbackEffect::Paused;
    qCDebug(ngflc) << "Effect paused, id" << id;
}

bool NGFFeedback::play(QFeedbackEffect::Effect effect)
{
    quint32 id;
    switch (effect) {
    case QFeedbackEffect::Press:
    case QFeedbackEffect::Release:
    case QFeedbackEffect::PressWeak:
    case QFeedbackEffect::ReleaseWeak:
    case QFeedbackEffect::PressStrong:
    case QFeedbackEffect::ReleaseStrong:
    case QFeedbackEffect::DragStart:
    case QFeedbackEffect::DragDropInZone:
    case QFeedbackEffect::DragDropOutOfZone:
    case QFeedbackEffect::DragCrossBoundary:
        /* These are quick effects and the status can not be retrieved
         * via effectState() anyway, thus this is not storing the id.
         */
        id = m_client.play(m_effects[effect]);
        if (!id)
            qCWarning(ngflc) << "Could not play effect";
        qCDebug(ngflc) << "Playing effect #" << effect << "(" << m_effects[effect] << ") with id" << id;
        return true;
    case QFeedbackEffect::Appear:
    case QFeedbackEffect::Disappear:
    case QFeedbackEffect::Move:
        qCDebug(ngflc) << "Unsupported effect #" << effect;
        break;
    default:
        qCDebug(ngflc) << "Unknown or undefined effect #" << effect;
        break;
    }

    return false;
}

QFeedbackInterface::PluginPriority NGFFeedback::pluginPriority()
{
    return QFeedbackInterface::PluginLowPriority;
}

QList<QFeedbackActuator*> NGFFeedback::actuators()
{
    return QList<QFeedbackActuator*>() << m_actuator;
}

void NGFFeedback::setActuatorProperty(const QFeedbackActuator &, ActuatorProperty prop, const QVariant &value)
{
    if (prop == ActuatorProperty::Enabled) {
        bool old = m_actuatorEnabled;
        m_actuatorEnabled = value.toBool();
        if (old != m_actuatorEnabled && !m_actuatorEnabled) {
            // Stop all effects
            for (auto it = m_activeEffects.begin(); it != m_activeEffects.end(); it = m_activeEffects.erase(it)) {
                if (!m_client.stop(it->id)) {
                    qCWarning(ngflc) << "Could not stop effect with id" << it->id;
                    if (it->effect)
                        reportError(it->effect, QFeedbackEffect::UnknownError);
                }
            }
            qCDebug(ngflc) << "Stopped all effects";
        }
    }
}

QVariant NGFFeedback::actuatorProperty(const QFeedbackActuator &, ActuatorProperty prop)
{
    switch (prop) {
    case Name:
        return QLatin1String("NGFD");
    case State:
        return QFeedbackActuator::Ready;
    case Enabled:
        return m_actuatorEnabled;
    default:
        return QVariant();
    }
}

bool NGFFeedback::isActuatorCapabilitySupported(const QFeedbackActuator &, QFeedbackActuator::Capability)
{
    /* Changing envelope or periodicity is not supported
     * since we don't support changing intensity level.
     */
    return false;
}

void NGFFeedback::updateEffectProperty(const QFeedbackHapticsEffect *effect, QFeedbackHapticsInterface::EffectProperty prop)
{
    if (!m_actuatorEnabled)
        return;

    ActiveEffect *active = findCustomEffect(effect);
    if (!active)
        return;

    if (prop == QFeedbackHapticsInterface::Duration) {
        qCDebug(ngflc) << "Playing custom effect due to property update (" << effect->duration() << "ms)";
        setEffectState(effect, QFeedbackEffect::Running);
    }
}

void NGFFeedback::setEffectState(const QFeedbackHapticsEffect *effect, QFeedbackEffect::State state)
{
    if (!m_actuatorEnabled)
        return;

    ActiveEffect *active = findCustomEffect(effect);

    switch (state) {
    case QFeedbackEffect::Running:
        if (active && active->state == QFeedbackEffect::Paused)
            resumeCustomEffect(active);
        else
            startCustomEffect(active, effect);
        break;
    case QFeedbackEffect::Stopped:
        stopCustomEffect(active);
        break;
    case QFeedbackEffect::Paused:
        pauseCustomEffect(active);
        break;
    case QFeedbackEffect::Loading:
    default:
        // not supported
        break;
    }
}

QFeedbackEffect::State NGFFeedback::effectState(const QFeedbackHapticsEffect *effect)
{
    ActiveEffect *active = findCustomEffect(effect);
    if (active)
        return active->state;
    return QFeedbackEffect::Stopped;
}

NGFFeedback::ActiveEffect *NGFFeedback::findEffect(quint32 id)
{
    // Assuming that there aren't too many effects
    for (ActiveEffect &active : m_activeEffects)
        if (active.id == id)
            return &active;
    return nullptr;
}

NGFFeedback::ActiveEffect *NGFFeedback::findCustomEffect(const QFeedbackHapticsEffect *effect)
{
    // Assuming that there aren't too many effects
    for (ActiveEffect &active : m_activeEffects)
        if (active.effect == effect)
            return &active;
    return nullptr;
}

void NGFFeedback::startCustomEffect(ActiveEffect *active, const QFeedbackHapticsEffect *effect)
{
    if (effect->duration() > 0) {
        qCDebug(ngflc) << "Playing custom effect due to state change (" << effect->duration() << "ms)";
        QMap<QString, QVariant> properties;
        properties.insert(QStringLiteral("haptic.duration"),
                          QVariant(static_cast<quint32>(effect->duration())));
        if (active) { // Existing effect
            m_client.stop(active->id);
            m_activeEffects.removeAll(*active);
        }
        /* The choice of the effect will only affect the strength and style
         * of the feedback. Duration is determined by haptic.duration property
         * which either repeats or "stretches" the effect to the whole duration.
         */
        quint32 id = m_client.play(QStringLiteral("feedback_alert"), properties);
        if (!id) {
            qCWarning(ngflc) << "Could not play effect";
            reportError(effect, QFeedbackEffect::UnknownError);
        } else {
            // Report already the expected state
            m_activeEffects.append({ id, QFeedbackEffect::Running, const_cast<QFeedbackHapticsEffect *>(effect) });
        }
    }
}

void NGFFeedback::stopCustomEffect(ActiveEffect *active)
{
    if (active) {
        qCDebug(ngflc) << "Stopping custom effect due to state change";
        if (!m_client.stop(active->id)) {
            qCWarning(ngflc) << "Could not stop effect with id" << active->id;
            m_activeEffects.removeAll(*active);
            reportError(active->effect, QFeedbackEffect::UnknownError);
        } else {
            // Report already the expected state
            active->state = QFeedbackEffect::Stopped;
        }
    }
}

void NGFFeedback::pauseCustomEffect(ActiveEffect *active)
{
    if (active) {
        qCDebug(ngflc) << "Pausing custom effect due to state change";
        if (!m_client.pause(active->id)) {
            qCWarning(ngflc) << "Could not pause effect with id" << active->id;
            reportError(active->effect, QFeedbackEffect::UnknownError);
        } else {
            // Report already the expected state
            active->state = QFeedbackEffect::Paused;
        }
    }
}

void NGFFeedback::resumeCustomEffect(ActiveEffect *active)
{
    if (active) {
        qCDebug(ngflc) << "Resuming custom effect due to state change";
        if (!m_client.resume(active->id)) {
            qCWarning(ngflc) << "Could not resume effect with id" << active->id;
            reportError(active->effect, QFeedbackEffect::UnknownError);
        } else {
            // Report already the expected state
            active->state = QFeedbackEffect::Running;
        }
    }
}

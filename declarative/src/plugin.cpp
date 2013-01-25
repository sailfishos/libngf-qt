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

#include <QtGlobal>
#include <QtDeclarative>
#include <QDeclarativeEngine>
#include <QDeclarativeExtensionPlugin>
#include "declarativengfevent.h"

class Q_DECL_EXPORT NgfPlugin : public QDeclarativeExtensionPlugin
{
public:
    virtual ~NgfPlugin() { }

    void initializeEngine(QDeclarativeEngine *engine, const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("org.nemomobile.ngf"));
        Q_UNUSED(uri);
        Q_UNUSED(engine);
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("org.nemomobile.ngf"));

        qmlRegisterType<DeclarativeNgfEvent>(uri, 1, 0, "NonGraphicalFeedback");
    }
};

Q_EXPORT_PLUGIN2(ngfplugin, NgfPlugin);


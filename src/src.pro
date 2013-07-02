include(../common.pri)

TEMPLATE = lib
VERSION = 0.1

QT -= gui
QT += core dbus

TARGET = ngf-qt$${NODASH_QT_VERSION}
DEFINES += NGFCLIENT_LIBRARY
CONFIG += create_pc create_prl no_install_prl

INCLUDEPATH += include
include(dbus/dbus.pri)

target.path = $$PREFIX/lib
headers.path = $$PREFIX/include/ngf-qt$${NODASH_QT_VERSION}
headers.files = include/*

QMAKE_PKGCONFIG_NAME = libngf-qt$${NODASH_QT_VERSION}
QMAKE_PKGCONFIG_PREFIX = $$PREFIX
QMAKE_PKGCONFIG_DESCRIPTION = "Qt-based client library for NGF daemon."
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers


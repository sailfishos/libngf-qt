include(../common.pri)

TEMPLATE = lib

QT -= gui
QT += core dbus

TARGET = ngf-qt$${QT_MAJOR_VERSION}
DEFINES += NGFCLIENT_LIBRARY
CONFIG += create_pc create_prl no_install_prl

INCLUDEPATH += include
include(dbus/dbus.pri)

target.path = $$[QT_INSTALL_LIBS]
headers.path = $$PREFIX/include/ngf-qt$${QT_MAJOR_VERSION}
headers.files = include/*

QMAKE_PKGCONFIG_NAME = libngf-qt$${QT_MAJOR_VERSION}
QMAKE_PKGCONFIG_PREFIX = $$PREFIX
QMAKE_PKGCONFIG_DESCRIPTION = "Qt-based client library for NGF daemon."
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers


TEMPLATE = lib
VERSION = 0.1

QT -= gui
QT += core dbus

equals(QT_MAJOR_VERSION, 4): TARGET = ngf-qt
equals(QT_MAJOR_VERSION, 5): TARGET = ngf-qt5
DEFINES += NGFCLIENT_LIBRARY
CONFIG += create_pc create_prl no_install_prl

OBJECTS_DIR = .obj
MOC_DIR = .moc

INCLUDEPATH += include
include(dbus/dbus.pri)

target.path = $$PREFIX/lib
equals(QT_MAJOR_VERSION, 4): headers.path = $$PREFIX/include/ngf-qt
equals(QT_MAJOR_VERSION, 5): headers.path = $$PREFIX/include/ngf-qt5
headers.files = include/*

equals(QT_MAJOR_VERSION, 4): QMAKE_PKGCONFIG_NAME = libngf-qt
equals(QT_MAJOR_VERSION, 5): QMAKE_PKGCONFIG_NAME = libngf-qt5
QMAKE_PKGCONFIG_PREFIX = $$PREFIX
QMAKE_PKGCONFIG_DESCRIPTION = "Qt-based client library for NGF daemon."
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers


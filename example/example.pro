TARGET = ngf-qt-client
TEMPLATE = app
CONFIG += ordered
QT -= gui
QT += core

INCLUDEPATH += ../src/include

target.path = $$PREFIX/bin

SOURCES += main.cpp
HEADERS += testing.h

LIBS += -L../src -lngf-qt

INSTALLS += target


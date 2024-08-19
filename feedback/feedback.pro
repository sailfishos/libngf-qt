TEMPLATE = lib

QT += core feedback
CONFIG += qt plugin hide_symbols
QT -= gui

TARGET = $$qtLibraryTarget(qtfeedback_libngf)
PLUGIN_TYPE = feedback

LIBS += -L../src/ -lngf-qt$${QT_MAJOR_VERSION}
INCLUDEPATH += ../src/include

HEADERS += ngffeedback.h

SOURCES += ngffeedback.cpp

OTHER_FILES += libngf.json

target.path = $$[QT_INSTALL_PLUGINS]/feedback
INSTALLS += target

plugindescription.files = libngf.json
plugindescription.path = $$[QT_INSTALL_PLUGINS]/feedback/
INSTALLS += plugindescription

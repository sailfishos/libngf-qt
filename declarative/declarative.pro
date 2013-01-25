TARGET = libngf-declarative
PLUGIN_IMPORT_PATH = org/nemomobile/ngf

LIBS += -L../src/ \
    -lngf-qt

INCLUDEPATH += ../src/include

SOURCES += src/plugin.cpp \
    src/declarativengfevent.cpp

HEADERS += src/declarativengfevent.h

TEMPLATE = lib
CONFIG += qt plugin hide_symbols
QT += declarative

target.path = $$[QT_INSTALL_IMPORTS]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$$$PLUGIN_IMPORT_PATH
INSTALLS += qmldir

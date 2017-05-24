include(../common.pri)

TARGET = ngf-declarative
PLUGIN_IMPORT_PATH = Nemo/Ngf

LIBS += -L../src/ -lngf-qt5

INCLUDEPATH += ../src/include

SOURCES += src/plugin.cpp \
    src/declarativengfevent.cpp

HEADERS += src/declarativengfevent.h

TEMPLATE = lib
CONFIG += qt plugin hide_symbols
QT += quick

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$target.path
INSTALLS += qmldir

qmltypes.files += $$PWD/plugins.qmltypes
qmltypes.path +=  $$target.path
INSTALLS += qmltypes

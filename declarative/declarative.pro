include(../common.pri)

TARGET = ngf-declarative
PLUGIN_IMPORT_PATH = Nemo/Ngf

LIBS += -L../src/ -lngf-qt$${QT_MAJOR_VERSION}

INCLUDEPATH += ../src/include

SOURCES += src/plugin.cpp \
           src/declarativengfevent.cpp \
           src/declarativengfeventproperty.cpp

HEADERS += src/declarativengfevent.h \
           src/declarativengfeventproperty.h

TEMPLATE = lib
CONFIG += qt plugin hide_symbols
QT += qml
QT -= gui

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += qmldir plugins.qmltypes
qmldir.path +=  $$target.path
INSTALLS += qmldir

qmltypes.commands = qmlplugindump -nonrelocatable Nemo.Ngf 1.0 > $$PWD/plugins.qmltypes
QMAKE_EXTRA_TARGETS += qmltypes

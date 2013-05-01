TARGET = ngf-declarative
PLUGIN_IMPORT_PATH = org/nemomobile/ngf

LIBS += -L../src/
equals(QT_MAJOR_VERSION, 4): LIBS += -lngf-qt
equals(QT_MAJOR_VERSION, 5): LIBS += -lngf-qt5

INCLUDEPATH += ../src/include

SOURCES += src/plugin.cpp \
    src/declarativengfevent.cpp

HEADERS += src/declarativengfevent.h

TEMPLATE = lib
CONFIG += qt plugin hide_symbols
equals(QT_MAJOR_VERSION, 4): QT += declarative
equals(QT_MAJOR_VERSION, 5): QT += quick

equals(QT_MAJOR_VERSION, 4): target.path = $$[QT_INSTALL_IMPORTS]/$$PLUGIN_IMPORT_PATH
equals(QT_MAJOR_VERSION, 5): target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$target.path
INSTALLS += qmldir

include(tests_common.pri)

pro_file_basename = $$basename(_PRO_FILE_)
pro_file_basename ~= s/\\.pro$//

TEMPLATE = app
TARGET = $${pro_file_basename}

QT += dbus testlib

HEADERS = testbase.h
SOURCES = $${pro_file_basename}.cpp

INCLUDEPATH += ../src/include
LIBS += -L../src -lngf-qt5

target.path = $${INSTALL_TESTDIR}
INSTALLS += target

check.depends = all
check.commands = 'false The variable check.commands must be set in $${_PRO_FILE_}'
check.CONFIG = phony
QMAKE_EXTRA_TARGETS += check

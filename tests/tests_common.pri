include(../common.pri)

INSTALL_TESTDIR = /opt/tests/libngf-qt$${QT_MAJOR_VERSION}
CONFIG_SUBST += INSTALL_TESTDIR
QT -= gui

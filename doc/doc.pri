doc.target = doc
doc.commands = test -d doc/man || doxygen doc/Doxyfile
doc.depends = FORCE

QMAKE_EXTRA_TARGETS += doc

manpage.files = doc/man/man3/*
manpage.path = $$PREFIX/share/man/man3
manpage.CONFIG += no_check_exist
manpage.depends = doc

INSTALLS += manpage


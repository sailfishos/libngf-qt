doc.target = doc
doc.commands = test -d doc/man || doxygen doc/Doxyfile
doc.depends = FORCE

QMAKE_EXTRA_TARGETS += doc


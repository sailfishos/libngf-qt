isEmpty(PREFIX) {
    PREFIX = /usr/local
}
TEMPLATE = subdirs
SUBDIRS += src declarative feedback

declarative.depends = src
tests.depends = src declarative
feedback.depends = src

# No need to build this, but if you want then 'qmake EXAMPLE=1 && make'
count(EXAMPLE, 1) {
    SUBDIRS += example
}

count(TEST, 1) {
    SUBDIRS += tests
}

include(doc/doc.pri)

# adds 'coverage' make target
include(coverage.pri)

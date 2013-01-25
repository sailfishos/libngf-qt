CONFIG += ordered
isEmpty(PREFIX) {
    PREFIX = /usr/local
}
TEMPLATE = subdirs
SUBDIRS += src declarative

# No need to build this, but if you want then 'qmake EXAMPLE=1 && make'
count(EXAMPLE, 1) {
    SUBDIRS += example
}

include(doc/doc.pri)


include(testapplication.pri)

QT += quick

check.commands = '\
    cd "$${OUT_PWD}" \
    && mkdir -p ../declarative/org/nemomobile \
    && ln -sfn ../.. ../declarative/org/nemomobile/ngf \
    && cp $${PWD}/../declarative/qmldir ../declarative \
    && export QML_IMPORT_PATH="$${OUT_PWD}/../declarative/" \
    && export LD_LIBRARY_PATH="$${OUT_PWD}/../src:\$\${LD_LIBRARY_PATH}" \
    && dbus-launch ./$${TARGET}'

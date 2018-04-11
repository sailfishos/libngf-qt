include(testapplication.pri)

QT += qml

check.commands = '\
    cd "$${OUT_PWD}" \
    && mkdir -p ../declarative/Nemo \
    && ln -sfn ../.. ../declarative/Nemo/Ngf \
    && cp $${PWD}/../declarative/qmldir ../declarative \
    && export QML_IMPORT_PATH="$${OUT_PWD}/../declarative/" \
    && export LD_LIBRARY_PATH="$${OUT_PWD}/../src:\$\${LD_LIBRARY_PATH}" \
    && dbus-launch ./$${TARGET}'

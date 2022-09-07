QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/deploy/qgroundcontrol-start.sh $$DESTDIR
QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/deploy/qgroundcontrol.desktop $$DESTDIR

QT += core gui widgets

TEMPLATE = app
TARGET = UpgradeGui

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    upgradeworker.cpp

HEADERS += \
    mainwindow.h \
    upgradeworker.h

FORMS += \
    mainwindow.ui

# Absolute path to backend binary
BACKEND_SOURCE = /home/oskars/RosUpgradeGUI/upgrade
BACKEND_DEST = $$OUT_PWD/upgrade

# Copy backend after build (Post build step)
QMAKE_POST_LINK += echo "Copying backend..." && cp -f $$BACKEND_SOURCE $$BACKEND_DEST && chmod +x $$BACKEND_DEST

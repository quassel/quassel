TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += qxt
QXT += core gui

FORMS += mainwindow.ui tab.ui
HEADERS += mainwindow.h tab.h
SOURCES += main.cpp mainwindow.cpp tab.cpp
RESOURCES += app.qrc

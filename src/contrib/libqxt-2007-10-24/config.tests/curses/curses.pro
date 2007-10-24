win32:include(../../depends.pri)
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
SOURCES += main.cpp
LIBS += -lpanel -lncurses
CONFIG -= qt
CONFIG -= app_bundle

win32:include(../../depends.pri)
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
SOURCES += main.cpp
LIBS+=-lavcodec -lavformat
QT=core
CONFIG -= app_bundle

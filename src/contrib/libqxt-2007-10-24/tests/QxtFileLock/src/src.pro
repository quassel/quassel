SOURCES += main.cpp \
threadtestcontroller.cpp \
locktestclient.cpp \
HelperClass.cpp
TEMPLATE = app
CONFIG += warn_on \
	  thread \
          qt \
	  debug
TARGET = ../bin/qxtfilelock

QT -= gui
QT += core \
network
CONFIG += qxt
CONFIG += console
QXT += core
HEADERS += threadtestcontroller.h \
locktestclient.h \
HelperClass.h

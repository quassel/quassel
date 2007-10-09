# Modules for monolithic client

TARGET  = quassel
MODULES = core qtgui client common contrib/qxt
DEFINES = BUILD_MONO

QT += network sql

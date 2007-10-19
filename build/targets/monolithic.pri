# Modules for monolithic client

TARGET  = quassel
MODULES = core qtui client common contrib/qxt
DEFINES = BUILD_MONO

QT += network sql

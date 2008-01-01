# Modules for monolithic client

TARGET  = quassel
MODULES = core qtui uisupport client common
DEFINES = BUILD_MONO

QT += network sql script

RESOURCES *= ../../src/icons/icons.qrc

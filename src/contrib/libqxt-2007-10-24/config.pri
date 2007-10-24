# This file needs to be copied into src/contrib/libqxt-$$QXTVER/config.pri if you
# update the snapshot!

# Stuff that is used by the libqxt project files
CONFIG += static staticlibs
CONFIG += release
DEFINES *= HAVE_QT
QXTBUILDDIR = ../../../../../build/contrib
DESTDIR = $$QXTBUILDDIR
MOC_DIR = $$QXTBUILDDIR/.libqxt/moc
OBJECTS_DIR = $$QXTBUILDDIR/.libqxt/obj

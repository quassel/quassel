# module.pri
# This file is included by module project files.

TEMPLATE = lib
CONFIG += staticlib

SRCPATH = ../../src  # Path to sources relative to this file

# Set paths according to MODULE
# We need to handle MODULE definitions like contrib/foo

MODNAME = $$basename(MODULE)
MODPATH_PREFIX = $$dirname(MODULE)
!isEmpty(MODPATH_PREFIX) {
  MODPATH_PREFIX ~= s,[^/]+,..
  #sprintf($$MODPATH_PREFIX%1
  SRCPATH = $$MODPATH_PREFIX/$$SRCPATH
}

MODPATH = $$SRCPATH/$$MODULE  # Path to the module files

# Define build directories

OBJECTS_DIR = .$$MODNAME
MOC_DIR =     .$$MODNAME
UI_DIR =      .$$MODNAME

# Load module settings (files etc.)

include($$MODPATH/$${MODNAME}.pri)

# Define needed Qt modules

QT -= gui
for(qtmod, QT_MOD) {
  QT *= $$qtmod
}

# Set includepath for needed Quassel modules

for(dep, DEPMOD) {
  INCLUDEPATH *= $$SRCPATH/$$dep
}
INCLUDEPATH *= $$MODPATH  # and don't forget our own dir

# Now prefix all filenames with the correct dirname

for(src, SRCS) {
  SOURCES *= $$MODPATH/$$src
}

for(hdr, HDRS) {
  HEADERS *= $$MODPATH/$$hdr
}

for(frm, FRMS) {
  FORMS *= $$MODPATH/$$frm
}

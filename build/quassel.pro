# This project file can be used to set up a build environment for quassel.
# To build the default configuration (i.e. qtgui, core and monolithic client),
# simply run
#
# > qmake && make
#
# in this directory. In order to select the binaries to build, you may set
# the BUILD variable as follows:
#
# > qmake BUILD=<mode> && make
#
# where <mode> is a quoted string that may contain any of 'qtgui', 'core', 'mono' or 'all'.
#
# NOTE: To change the build configuration, you have to run 'make distclean' first!


# Set project-wide config options

#CONFIG = qt warn_on release

TEMPLATE = subdirs

# Check build configuration
isEmpty(BUILD) {
  BUILD = all  # build everything by default
}

contains(BUILD, all) {
  BUILD += qtclient core mono
}

contains(BUILD, mono) {
  include(targets/monolithic.pri)
  BUILD_MODS *= $${MODULES}
  BUILD_TARGETS *= monolithic
}

contains(BUILD, core) {
  include(targets/core.pri)
  BUILD_MODS *= $${MODULES}
  BUILD_TARGETS *= core
}

contains(BUILD, qtclient) {
  include(targets/qtclient.pri)
  BUILD_MODS *= $${MODULES}
  BUILD_TARGETS *= qtclient
}

# First we build contrib stuff...
# SUBDIRS += contrib/libqxt.pro   # no deps to libqxt at the moment

# Then we build all needed modules...
for(mod, BUILD_MODS) {
  SUBDIRS += modules/$${mod}.pro
}

# ... followed by the binaries.
for(target, BUILD_TARGETS) {
  SUBDIRS += targets/$${target}.pro
}

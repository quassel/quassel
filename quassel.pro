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

TEMPLATE = subdirs
SUBDIRS += build/quassel.pro 

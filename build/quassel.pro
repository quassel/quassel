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

TARGETS = qtclient core mono

# Check build configuration
isEmpty(BUILD): BUILD = all  # build everything by default
contains(BUILD, all): BUILD = $${TARGETS}

# Find modules and targets to build
for(target, TARGETS): contains(BUILD, $$target) {
  include(targets/$${target}.pri)
  BUILD_MODS *= $${MODULES}
  BUILD_TARGETS *= $$target
}

# Now add modules and their deps
for(mod, BUILD_MODS) {
  include(../src/$${mod}/$${mod}.pri)
  SUBDIRS += mod_$${mod}
  eval(mod_$${mod}.file = modules/$${mod}.pro)
  eval(mod_$${mod}.makefile = Makefile.mod_$${mod})  # This prevents distclean from removing our Makefile -_-
  for(dep, DEPMOD): eval(mod_$${mod}.depends += mod_$${dep})
  export(mod_$${mod}.file)
  export(mod_$${mod}.makefile)
  export(mod_$${mod}.depends)
}

# Same with targets
for(target, BUILD_TARGETS) {
  include(targets/$${target}.pri)
  SUBDIRS += $${target}
  eval($${target}.file = targets/$${target}.pro)
  eval($${target}.makefile = Makefile.target_$${target})
  for(mod, MODULES): eval($${target}.depends += mod_$${mod})
  export($${target}.file)
  export($${target}.makefile)
  export($${target}.depends)
}


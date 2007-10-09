isEmpty(BUILD) {
  BUILD = all
}
contains(BUILD, all) {
  BUILD += qtgui core mono
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

TEMPLATE = subdirs

for(mod, BUILD_MODS) {
  SUBDIRS += modules/$${mod}.pro
}

for(target, BUILD_TARGETS) {
  SUBDIRS += targets/$${target}.pro
}

CONFIG += qt warn_on

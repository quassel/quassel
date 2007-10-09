TEMPLATE = app

SRCPATH = ../../src
OBJECTS_DIR = .$$TARGET

for(mod, MODULES) {
  INCLUDEPATH *= $$SRCPATH/$$mod
  LIBS *= -L../modules/$$dirname(mod) -l$$basename(mod)
}

SOURCES = $$SRCPATH/common/main.cpp

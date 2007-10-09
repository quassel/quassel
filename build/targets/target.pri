TEMPLATE = app
SRCPATH = ../../src
OBJECTS_DIR = $${TARGET}.tmp

for(mod, MODULES) {
  INCLUDEPATH *= $$SRCPATH/$$mod
  LIBS *= -L../modules/$$dirname(mod) -l$$basename(mod)
}

SOURCES = $$SRCPATH/common/main.cpp

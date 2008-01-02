TEMPLATE = app

include(../buildconf.pri)

RESOURCES   *= ../../i18n/i18n.qrc
TRANSLATIONS = quassel_de.ts \
               quassel_da.ts

SRCPATH = ../../src
OBJECTS_DIR = .$$TARGET
RCC_DIR     = .$$TARGET

for(mod, MODULES) {
  INCLUDEPATH *= $$SRCPATH/$$mod
  LIBS *= -L../modules/$$dirname(mod) -l$$basename(mod)
  PRE_TARGETDEPS *= ../modules/$$mod
}

#CONTRIB += libqxt  # not needed
#include(../contrib/contrib.pri)

SOURCES = $$SRCPATH/common/main.cpp

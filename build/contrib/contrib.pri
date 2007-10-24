contains(CONTRIB, libqxt) {
  include(../contrib/libqxt-version.pri)
  QXTLIB = ../contrib
  QXTINC = ../../src/contrib/libqxt-$$QXTVER/deploy/include
  QXTREALINC = ../../src/contrib/libqxt-$$QXTVER/src
  INCLUDEPATH += $$QXTINC/QxtCore $$QXTINC/QxtNetwork $$QXTREALINC/network $$QXTREALINC/core
  LIBS += -L$$QXTLIB -lQxtNetwork -lQxtCore
}

TEMPLATE = subdirs

include(libqxt-version.pri)
QXTDIR = ../../src/contrib/libqxt-$$QXTVER/src

SUBDIRS = $$QXTDIR/core $$QXTDIR/network

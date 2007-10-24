TARGET          =  QxtNetwork
DESTDIR         =  ../../deploy/libs 
DEPENDPATH      += .
INCLUDEPATH     += . ../core
DEFINES         += BUILD_QXT_NETWORK
win32: CONFIG   += dll
QT               = core network
QXT              = core
INCLUDEPATH     += .
TEMPLATE         = lib
MOC_DIR          = .moc
OBJECTS_DIR      = .obj
CONFIG += qxtbuild  convenience
include(../../config.pri)



HEADERS +=  qxtrpcpeer.h
SOURCES += qxtrpcpeer.cpp


#HEADERS += qxtnamedpipe.h
#SOURCES += qxtnamedpipe.cpp

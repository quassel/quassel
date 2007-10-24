TARGET = QxtCrypto
DESTDIR = ../../deploy/libs/
DEPENDPATH += .
INCLUDEPATH     += . thirdparty ../core

DEFINES += BUILD_QXT_CRYPTO
win32: CONFIG += dll
QT = core
INCLUDEPATH += .
TEMPLATE = lib
MOC_DIR = .moc
OBJECTS_DIR = .obj
CONFIG += qxtbuild  convenience
include(../../config.pri)

SOURCES+= thirdparty/md5.cpp thirdparty/md4.cpp


HEADERS += qxthash.h 
SOURCES += qxthash.cpp


contains(DEFINES,HAVE_OPENSSL){
HEADERS += qxtblowfish.h
SOURCES += qxtblowfish.cpp
LIBS+=-lssl
}




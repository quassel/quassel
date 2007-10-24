TARGET          =  QxtWeb
#DESTDIR         = ../../deploy/libs
DEPENDPATH      += .
INCLUDEPATH     += . ../core ../network
DEFINES         += BUILD_QXT_WEB
win32: CONFIG   += dll
QT               = core network
QXT              = core network
INCLUDEPATH     += .
TEMPLATE         = lib
MOC_DIR          = .moc
OBJECTS_DIR      = .obj
CONFIG          += qxtbuild convenience
include(../../config.pri)


HEADERS+= qxthtmltemplate.h  qxtwebcore.h  qxtwebcontroller.h  qxtwebcore_p.h   qxtabstractwebconnector.h qxtscgiconnector.h
SOURCES+= qxthtmltemplate.cpp  qxtwebcore.cpp  qxtwebcontroller.cpp  qxtscgiconnector.cpp




contains(DEFINES,HAVE_FCGI){
HEADERS += qxtfcgiconnector.h qxtfcgiconnector_p.h
SOURCES += qxtfcgiconnector.cpp
LIBS+=-lfcgi++
}


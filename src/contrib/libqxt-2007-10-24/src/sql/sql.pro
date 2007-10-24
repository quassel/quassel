TARGET          =  QxtSql
#DESTDIR         = .lib
DEPENDPATH      += .
INCLUDEPATH     += . ../core
DEFINES         += BUILD_QXT_SQL
win32: CONFIG   += dll
QT               = core sql
INCLUDEPATH     += .
TEMPLATE         = lib
MOC_DIR          = .moc
OBJECTS_DIR      = .obj
CONFIG += qxtbuild  convenience
include(../../config.pri)

HEADERS += qxtsqlpackage.h   qxtsqlpackagemodel.h
SOURCES += qxtsqlpackage.cpp qxtsqlpackagemodel.cpp

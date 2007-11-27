qtopia_project(qtopia app)

TARGET = quasseltopia
CONFIG += debug qtopia_main no_quicklaunch no_singleexec
QT += core gui network

# Find files
INCLUDEPATH += ../src/qtopia ../src/uisupport ../src/client ../src/common

#DESTDIR = .
#OBJECTS_DIR = .obj
#MOC_DIR = .moc
#UIC_DIR = .ui

# Include .pri from src dirs
include(../src/common/common.pri)
include(../src/qtopia/qtopia.pri)
include(../src/client/client.pri)
include(../src/uisupport/uisupport.pri)

# Fix variable names
SOURCES = $$SRCS
HEADERS = $$HDRS
FORMS   = $$FRMS

# SXE permissions required
#pkg.domain=
#pkg.name=Quassel IRC

desktop.files=../src/qtopia/quasseltopia.desktop
desktop.path=/apps/Applications
#desktop.trtarget=example-nct
desktop.hint=nct desktop

pics.files=../src/images/qirc-icon.png
pics.path=/pics/quasselirc/
pics.hint=pics

#help.source=help
#help.files=example.html
#help.hint=help

INSTALLS+=desktop

pkg.name=QuasselTopia
pkg.desc=Quassel IRC, a next-gen IRC client
pkg.version=0.0.1-1
pkg.maintainer=www.quassel-irc.org
pkg.license=GPL
pkg.domain=window,net


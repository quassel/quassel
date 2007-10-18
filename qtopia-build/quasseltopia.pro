qtopia_project(qtopia app)

TARGET=quasseltopia
CONFIG+=debug qtopia_main no_quicklaunch no_singleexec no_sxe_test
QT = core gui network

# Find files
INCLUDEPATH+=../src/qtopia ../src/client ../src/common ../src/contrib/qxt


# Include .pri from src dirs
include(../src/contrib/qxt/qxt.pri)
include(../src/common/common.pri)
include(../src/qtopia/qtopia.pri)
include(../src/client/client.pri)

# Fix variable names
SOURCES = $$SRCS
HEADERS = $$HDRS
FORMS   = $$FRMS

# SXE permissions required
#pkg.domain=none
#pkg.name=Quassel IRC

desktop.files=../src/qtopia/quasseltopia.desktop
desktop.path=/apps/Applications
desktop.trtarget=example-nct
desktop.hint=nct desktop

#pics.files=pics/*
#pics.path=/pics/example
#pics.hint=pics

#help.source=help
#help.files=example.html
#help.hint=help

INSTALLS+=desktop

pkg.name=QuasselTopia
pkg.desc=Quassel IRC, a next-gen IRC client
pkg.version=0.0.1-1
pkg.maintainer=www.quassel-irc.org
pkg.license=GPL
pkg.domain=window

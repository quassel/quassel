DEPMOD = uisupport common client
QT_MOD = core gui network

HDRS += bufferviewwidget.h chatline.h chatwidget.h coreconnectdlg.h mainwidget.h nicklistwidget.h \
        qtopiaui.h qtopiamainwin.h qtopiauistyle.h topicbar.h
SRCS += bufferviewwidget.cpp chatline.cpp chatwidget.cpp coreconnectdlg.cpp main.cpp mainwidget.cpp nicklistwidget.cpp \
        qtopiaui.cpp qtopiamainwin.cpp qtopiauistyle.cpp topicbar.cpp
FRMS += ui/bufferviewwidget.ui ui/coreconnectdlg.ui ui/coreconnectprogressdlg.ui ui/editcoreacctdlg.ui ui/mainwidget.ui ui/nicklistwidget.ui

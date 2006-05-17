SET(gui_SRCS channelwidget.cpp mainwin.cpp serverlist.cpp)
SET(gui_HDRS )
SET(gui_MOCS channelwidget.h mainwin.h serverlist.h)
SET(gui_UICS channelwidget.ui identitiesdlg.ui identitieseditdlg.ui networkeditdlg.ui nickeditdlg.ui serverlistdlg.ui)

QT4_WRAP_UI(_UIC ${gui_UICS})
QT4_WRAP_CPP(_MOC ${gui_MOCS})

ADD_LIBRARY(gui ${_UIC} ${_MOC} ${gui_HDRS} ${gui_SRCS})

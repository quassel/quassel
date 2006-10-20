SET(gui_SRCS channelwidget.cpp mainwin.cpp serverlist.cpp coreconnectdlg.cpp guiproxy.cpp)
SET(gui_HDRS )
SET(gui_MOCS channelwidget.h mainwin.h serverlist.h coreconnectdlg.h guiproxy.h)
SET(gui_UICS channelwidget-old.ui identitiesdlg.ui identitieseditdlg.ui networkeditdlg.ui nickeditdlg.ui serverlistdlg.ui coreconnectdlg.ui)

QT4_WRAP_UI(_UIC ${gui_UICS})
QT4_WRAP_CPP(_MOC ${gui_MOCS})

# We need to work around a dependency bug with out-of-source builds...
# Seems to be fixed!
#SET_SOURCE_FILES_PROPERTIES(${gui_SRCS} PROPERTIES OBJECT_DEPENDS "${_UIC}")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(gui ${gui_HDRS} ${gui_SRCS} ${_MOC} ${_UIC})

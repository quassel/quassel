SET(gui_SRCS channelwidget.cpp mainwin.cpp serverlist.cpp)
SET(gui_HDRS )
SET(gui_MOCS channelwidget.h mainwin.h serverlist.h)
SET(gui_UICS channelwidget.ui identitiesdlg.ui identitieseditdlg.ui networkeditdlg.ui nickeditdlg.ui serverlistdlg.ui)

QT4_WRAP_UI(_UIC ${gui_UICS})
QT4_WRAP_CPP(_MOC ${gui_MOCS})

# We need to workaround a dependency bug with out-of-source builds...
SET_SOURCE_FILES_PROPERTIES(${gui_SRCS} PROPERTIES
OBJECT_DEPENDS "${_UIC}")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(gui ${gui_HDRS} ${gui_SRCS} ${_MOC} ${_UIC})

SET(gui_SRCS channelwidgetinput.cpp mainwin.cpp serverlist.cpp buffer.cpp
             identities.cpp coreconnectdlg.cpp guiproxy.cpp networkview.cpp)
SET(gui_HDRS )
SET(gui_MOCS channelwidgetinput.h mainwin.h serverlist.h identities.h coreconnectdlg.h
             guiproxy.h networkview.h buffer.h)
SET(gui_UICS identitiesdlg.ui identitieseditdlg.ui networkeditdlg.ui mainwin.ui
             nickeditdlg.ui serverlistdlg.ui servereditdlg.ui coreconnectdlg.ui ircwidget.ui
             networkview.ui bufferwidget.ui)

# This prepends ui/ to the UIC names, so we don't have to do this ourselves.
FOREACH(ui ${gui_UICS})
  SET(gui_UICS_UI ${gui_UICS_UI} "ui/${ui}")
ENDFOREACH(ui)

QT4_WRAP_UI(_UIC ${gui_UICS_UI})
QT4_WRAP_CPP(_MOC ${gui_MOCS})

# We need to work around a dependency bug with out-of-source builds...
# Seems to be fixed!
#SET_SOURCE_FILES_PROPERTIES(${gui_SRCS} PROPERTIES OBJECT_DEPENDS "${_UIC}")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(gui ${gui_HDRS} ${gui_SRCS} ${_MOC} ${_UIC})

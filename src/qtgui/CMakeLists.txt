SET(qtgui_SRCS chatwidget.cpp chatline.cpp channelwidgetinput.cpp tabcompleter.cpp mainwin.cpp serverlist.cpp bufferwidget.cpp
             identities.cpp coreconnectdlg.cpp bufferview.cpp bufferviewwidget.cpp bufferviewfilter.cpp style.cpp settingsdlg.cpp settingspages.cpp)
SET(qtgui_HDRS style.h)
SET(qtgui_MOCS chatwidget.h chatline.h channelwidgetinput.h tabcompleter.h mainwin.h serverlist.h identities.h coreconnectdlg.h
             bufferview.h bufferwidget.h bufferviewwidget.h bufferviewfilter.h settingsdlg.h settingspages.h)
SET(qtgui_UICS identitiesdlg.ui identitieseditdlg.ui networkeditdlg.ui mainwin.ui
             nickeditdlg.ui serverlistdlg.ui servereditdlg.ui coreconnectdlg.ui ircwidget.ui
             bufferviewwidget.ui bufferwidget.ui settingsdlg.ui
             buffermgmntsettingspage.ui connectionsettingspage.ui)

# This prepends ui/ to the UIC names, so we don't have to do this ourselves.
FOREACH(ui ${qtgui_UICS})
  SET(qtgui_UICS_UI ${qtgui_UICS_UI} "ui/${ui}")
ENDFOREACH(ui)

QT4_WRAP_UI(_UIC ${qtgui_UICS_UI})
QT4_WRAP_CPP(_MOC ${qtgui_MOCS})

# We need to work around a dependency bug with out-of-source builds...
# Seems to be fixed!
#SET_SOURCE_FILES_PROPERTIES(${gui_SRCS} PROPERTIES OBJECT_DEPENDS "${_UIC}")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(qtgui ${qtgui_HDRS} ${qtgui_SRCS} ${_MOC} ${_UIC})
TARGET_LINK_LIBRARIES(qtgui common)
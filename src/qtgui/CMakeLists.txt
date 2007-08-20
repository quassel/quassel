SET(qtgui_SRCS bufferview.cpp bufferviewfilter.cpp bufferwidget.cpp channelwidgetinput.cpp chatline.cpp chatwidget.cpp coreconnectdlg.cpp
             guisettings.cpp identities.cpp mainwin.cpp qtgui.cpp serverlist.cpp settingsdlg.cpp style.cpp tabcompleter.cpp)
SET(qtgui_HDRS style.h settingspage.h)
SET(qtgui_MOCS bufferview.h bufferviewfilter.h bufferwidget.h channelwidgetinput.h chatline.h chatwidget.h coreconnectdlg.h
              guisettings.h identities.h mainwin.h qtgui.h serverlist.h settingsdlg.h tabcompleter.h)
SET(qtgui_UICS identitiesdlg.ui identitieseditdlg.ui networkeditdlg.ui mainwin.ui
             nickeditdlg.ui serverlistdlg.ui servereditdlg.ui coreconnectdlg.ui
             bufferviewwidget.ui bufferwidget.ui settingsdlg.ui
             buffermgmtpage.ui connectionpage.ui usermgmtpage.ui)

# This prepends ui/ to the UIC names, so we don't have to do this ourselves.
FOREACH(ui ${qtgui_UICS})
  SET(qtgui_UICS_UI ${qtgui_UICS_UI} "ui/${ui}")
ENDFOREACH(ui)

QT4_WRAP_UI(_UIC ${qtgui_UICS_UI})
QT4_WRAP_CPP(_MOC ${qtgui_MOCS})

#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(qtgui ${qtgui_SRCS} ${_MOC} ${_UIC})
TARGET_LINK_LIBRARIES(qtgui common client)
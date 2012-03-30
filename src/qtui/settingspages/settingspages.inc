# Putting $FOO in SETTINGSPAGES automatically includes
# $FOOsettingspage.cpp, $FOOsettingspage.h and $FOOsettingspage.ui

set(SETTINGSPAGES aliases appearance backlog bufferview chatview connection coreconnection chatmonitor coreaccount
                  highlight identities ignorelist inputwidget itemview networks topicwidget)

# Specify additional files (e.g. for subdialogs) here!
set(SP_SOURCES aliasesmodel.cpp identityeditwidget.cpp ignorelistmodel.cpp notificationssettingspage.cpp previewbufferview.cpp)
set(SP_HEADERS aliasesmodel.h identityeditwidget.h ignorelistmodel.h notificationssettingspage.h previewbufferview.h)
set(SP_FORMS buffervieweditdlg.ui coreaccounteditdlg.ui createidentitydlg.ui identityeditwidget.ui ignorelisteditdlg.ui saveidentitiesdlg.ui 
             networkadddlg.ui networkeditdlg.ui nickeditdlg.ui servereditdlg.ui)

if(NOT HAVE_KDE)
  set(SETTINGSPAGES ${SETTINGSPAGES} shortcuts)
  set(SP_SOURCES ${SP_SOURCES} keysequencewidget.cpp shortcutsmodel.cpp)
  set(SP_HEADERS ${SP_HEADERS} keysequencewidget.h shortcutsmodel.h)
endif(NOT HAVE_KDE)

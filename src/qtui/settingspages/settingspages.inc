# Putting $FOO in SETTINGSPAGES automatically includes
# $FOOsettingspage.cpp, $FOOsettingspage.h and $FOOsettingspage.ui

set(SETTINGSPAGES aliases appearance backlog bufferview color connection chatmonitor general highlight identities networks)

# Specify additional files (e.g. for subdialogs) here!
set(SP_SOURCES aliasesmodel.cpp identityeditwidget.cpp notificationssettingspage.cpp)
set(SP_HEADERS aliasesmodel.h identityeditwidget.h notificationssettingspage.h previewbufferview.h)
set(SP_FORMS buffervieweditdlg.ui createidentitydlg.ui identityeditwidget.ui saveidentitiesdlg.ui networkadddlg.ui networkeditdlg.ui nickeditdlg.ui servereditdlg.ui)

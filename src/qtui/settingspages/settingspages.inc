# Putting $FOO in SETTINGSPAGES automatically includes
# $FOOsettingspage.cpp, $FOOsettingspage.h and $FOOsettingspage.ui

set(SETTINGSPAGES aliases appearance backlog bufferview color chatmonitor fonts general highlight identities networks)

# Specify additional files (e.g. for subdialogs) here!
set(SP_SOURCES aliasesmodel.cpp notificationssettingspage.cpp)
set(SP_HEADERS aliasesmodel.h notificationssettingspage.h previewbufferview.h)
set(SP_FORMS buffervieweditdlg.ui createidentitydlg.ui saveidentitiesdlg.ui networkadddlg.ui networkeditdlg.ui nickeditdlg.ui servereditdlg.ui)

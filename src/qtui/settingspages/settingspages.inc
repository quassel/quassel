# Putting $FOO in SETTINGSPAGES automatically includes
# $FOOsettingspage.cpp, $FOOsettingspage.h and $FOOsettingspage.ui

set(SETTINGSPAGES aliases appearance bufferview color fonts general highlight identities networks notifications)

# Specify additional files (e.g. for subdialogs) here!
set(SP_SOURCES aliasesmodel.cpp)
set(SP_HEADERS aliasesmodel.h)
set(SP_FORMS buffervieweditdlg.ui createidentitydlg.ui saveidentitiesdlg.ui networkeditdlg.ui nickeditdlg.ui servereditdlg.ui)

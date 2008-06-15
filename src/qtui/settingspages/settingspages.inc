# Putting $FOO in SETTINGSPAGES automatically includes
# $FOOsettingspage.cpp, $FOOsettingspage.h and $FOOsettingspage.ui
set(SETTINGSPAGES appearance bufferview color fonts general highlight identities networks)

# Specify additional files (e.g. for subdialogs) here!
set(SP_SOURCES )
set(SP_HEADERS )
set(SP_FORMS buffervieweditdlg.ui createidentitydlg.ui saveidentitiesdlg.ui networkeditdlg.ui nickeditdlg.ui servereditdlg.ui)

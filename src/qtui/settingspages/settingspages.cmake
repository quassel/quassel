# Putting $FOO in SETTINGSPAGES automatically includes
# $FOOsettingspage.cpp, $FOOsettingspage.h and $FOOsettingspage.ui

set(SETTINGSPAGES
    aliases
    appearance
    backlog
    bufferview
    chatmonitor
    chatview
    connection
    coreconnection
    coreaccount
    highlight
    identities
    ignorelist
    inputwidget
    itemview
    networks
    topicwidget
)

# Specify additional files (e.g. for subdialogs) here!
set(SP_SOURCES
    aliasesmodel.cpp
    identityeditwidget.cpp
    ignorelistmodel.cpp
    notificationssettingspage.cpp

    # needed for automoc
    previewbufferview.h
)

set(SP_FORMS
    buffervieweditdlg.ui
    coreaccounteditdlg.ui
    createidentitydlg.ui
    identityeditwidget.ui
    ignorelisteditdlg.ui
    networkadddlg.ui
    networkeditdlg.ui
    nickeditdlg.ui
    saveidentitiesdlg.ui
    servereditdlg.ui
)

if(NOT HAVE_KDE)
  set(SETTINGSPAGES ${SETTINGSPAGES} shortcuts)
  set(SP_SOURCES ${SP_SOURCES} keysequencewidget.cpp shortcutsmodel.cpp)
endif(NOT HAVE_KDE)

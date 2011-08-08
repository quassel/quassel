#ifndef QUICKACCESSORSSETTINGSPAGE_H
#define QUICKACCESSORSSETTINGSPAGE_H

#include <QWidget>

#include "settingspage.h"
#include "shortcutssettingspage.h"

#include "action.h"
#include "actioncollection.h"
#include "types.h"
#include "bufferviewconfig.h"
#include "shortcutsmodel.h"

class QuickAccessorsModel : public ShortcutsModel {
    Q_OBJECT

public:
    QuickAccessorsModel(const QHash<QString, ActionCollection*> &colls, QWidget *parent = 0);
    void commit();
};

class QuickAccessorsSettingsPage : public ShortcutsSettingsPage
{
    Q_OBJECT

public:
    QuickAccessorsSettingsPage(QWidget *parent = 0);
    //~QuickAccessorsSettingsPage();

public slots:
    void load();

private:
    //Ui::QuickAccessorsSettingsPage ui;
    //BufferViewConfig *_config;
    //ShortcutsModel *_shortcutsModel;
    //ActionCollection *_actionCollection;
};

#endif // QUICKACCESSORSSETTINGSPAGE_H

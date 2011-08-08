#include "quickaccessorssettingspage.h"

#include "qtui.h"
#include "client.h"
#include "clientsettings.h"
#include "bufferviewconfig.h"
#include "buffermodel.h"
#include "buffersettings.h"
#include "bufferview.h"

QuickAccessorsSettingsPage::QuickAccessorsSettingsPage(QWidget *parent)
    : ShortcutsSettingsPage(QtUi::allActionCollections(), QtUi::quickAccessorActionCollections().keys(), parent, tr("Interface"), tr("Quick Accessors"))
{

}


void QuickAccessorsSettingsPage::load() {
    //We need to replace _shortcutsModel with our own class which saves things correctly.
    delete _shortcutsModel;

    _shortcutsModel = new QuickAccessorsModel(QtUi::allActionCollections(), this);
    _shortcutsModel->load();
    _shortcutsFilter->setSourceModel(_shortcutsModel);
    connect(_shortcutsModel, SIGNAL(hasChanged(bool)), SLOT(setChangedState(bool)));
    ui.keySequenceWidget->setModel(_shortcutsModel);

    ui.shortcutsView->expandAll();

    setChangedState(false);

    SettingsPage::load();
}



QuickAccessorsModel::QuickAccessorsModel(const QHash<QString, ActionCollection *> &colls, QWidget *parent)
    : ShortcutsModel(colls, parent)
{

}


void QuickAccessorsModel::commit() {
    BufferId id;
    foreach(Item *catItem, _categoryItems) {
        foreach(Item *actItem, catItem->actionItems) {
            // Save the shortcut to the buffer's settings.
            // NOTE: This could break if the user invalidates the BufferId in between
            // showing the settingspage and saving it. (Do we care?),
            id = qvariant_cast<BufferId>(actItem->action->property("BufferId"));
            BufferSettings s(id);
            s.setShortcut(actItem->shortcut);
        }
    }
    ShortcutsModel::commit();
}

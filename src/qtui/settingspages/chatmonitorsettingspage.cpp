/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "chatmonitorsettingspage.h"

#include <QMessageBox>
#include <QVariant>

#include "backlogrequester.h"
#include "backlogsettings.h"
#include "buffermodel.h"
#include "bufferview.h"
#include "bufferviewconfig.h"
#include "bufferviewfilter.h"
#include "chatviewsettings.h"
#include "client.h"
#include "icon.h"
#include "networkmodel.h"
#include "util.h"

ChatMonitorSettingsPage::ChatMonitorSettingsPage(QWidget* parent)
    : SettingsPage(tr("Interface"), tr("Chat Monitor"), parent)
{
    ui.setupUi(this);

    ui.activateBuffer->setIcon(icon::get("go-next"));
    ui.deactivateBuffer->setIcon(icon::get("go-previous"));

    // setup available buffers config (for the bufferview on the left)
    _configAvailable = new BufferViewConfig(-667, this);
    _configAvailable->setBufferViewName("tmpChatMonitorAvailableBuffers");
    _configAvailable->setSortAlphabetically(true);
    _configAvailable->setDisableDecoration(true);
    _configAvailable->setNetworkId(NetworkId());
    _configAvailable->setInitialized();

    // setup active buffers config (for the bufferview on the right)
    _configActive = new BufferViewConfig(-666, this);
    _configActive->setBufferViewName("tmpChatMonitorActiveBuffers");
    _configActive->setSortAlphabetically(true);
    _configActive->setDisableDecoration(true);
    _configActive->setNetworkId(NetworkId());
    _configActive->setInitialized();

    // fill combobox with operation modes
    ui.operationMode->addItem(tr("Opt In"), ChatViewSettings::OptIn);
    ui.operationMode->addItem(tr("Opt Out"), ChatViewSettings::OptOut);

    // connect slots
    connect(ui.operationMode, selectOverload<int>(&QComboBox::currentIndexChanged), this, &ChatMonitorSettingsPage::switchOperationMode);
    connect(ui.showHighlights, &QAbstractButton::toggled, this, &ChatMonitorSettingsPage::widgetHasChanged);
    connect(ui.showOwnMessages, &QAbstractButton::toggled, this, &ChatMonitorSettingsPage::widgetHasChanged);
    connect(ui.alwaysOwn, &QAbstractButton::toggled, this, &ChatMonitorSettingsPage::widgetHasChanged);
    connect(ui.showBacklog, &QAbstractButton::toggled, this, &ChatMonitorSettingsPage::widgetHasChanged);
    connect(ui.includeRead, &QAbstractButton::toggled, this, &ChatMonitorSettingsPage::widgetHasChanged);

    // AsNeededBacklogRequester conflicts with showing backlog in Chat Monitor
    BacklogSettings backlogSettings;
    backlogSettings.initAndNotify("RequesterType", this, &ChatMonitorSettingsPage::setRequesterType, BacklogRequester::AsNeeded);
}

bool ChatMonitorSettingsPage::hasDefaults() const
{
    return true;
}

void ChatMonitorSettingsPage::defaults()
{
    // NOTE: Whenever changing defaults here, also update ChatMonitorFilter::ChatMonitorFilter()
    // and ChatMonitorSettingsPage::loadSettings() to match

    settings["OperationMode"] = ChatViewSettings::OptOut;
    settings["ShowHighlights"] = false;
    settings["ShowOwnMsgs"] = true;
    settings["AlwaysOwn"] = false;
    settings["Buffers"] = QVariant();
    settings["Default"] = true;
    settings["ShowBacklog"] = true;
    settings["IncludeRead"] = false;
    load();
    widgetHasChanged();
}

void ChatMonitorSettingsPage::load()
{
    if (settings.contains("Default"))
        settings.remove("Default");
    else
        loadSettings();

    switchOperationMode(settings["OperationMode"].toInt() - 1);
    ui.operationMode->setCurrentIndex(settings["OperationMode"].toInt() - 1);
    ui.showHighlights->setChecked(settings["ShowHighlights"].toBool());
    ui.showOwnMessages->setChecked(settings["ShowOwnMsgs"].toBool());
    ui.alwaysOwn->setChecked(settings["AlwaysOwn"].toBool());
    ui.showBacklog->setChecked(settings["ShowBacklog"].toBool());
    ui.includeRead->setChecked(settings["IncludeRead"].toBool());

    // get all available buffer Ids
    QList<BufferId> allBufferIds = Client::networkModel()->allBufferIds();

    if (!settings["Buffers"].toList().isEmpty()) {
        QList<BufferId> bufferIdsFromConfig;
        // remove all active buffers from the available config
        foreach (QVariant v, settings["Buffers"].toList()) {
            bufferIdsFromConfig << v.value<BufferId>();
            allBufferIds.removeAll(v.value<BufferId>());
        }
        Client::networkModel()->sortBufferIds(bufferIdsFromConfig);
        _configActive->setBufferList(bufferIdsFromConfig);
    }
    ui.activeBuffers->setFilteredModel(Client::bufferModel(), _configActive);

    Client::networkModel()->sortBufferIds(allBufferIds);
    _configAvailable->setBufferList(allBufferIds);
    ui.availableBuffers->setFilteredModel(Client::bufferModel(), _configAvailable);

    setChangedState(false);
}

void ChatMonitorSettingsPage::loadSettings()
{
    // NOTE: Whenever changing defaults here, also update ChatMonitorFilter::ChatMonitorFilter()
    // and ChatMonitorSettingsPage::defaults() to match
    ChatViewSettings chatViewSettings("ChatMonitor");

    settings["OperationMode"] = (ChatViewSettings::OperationMode)chatViewSettings.value("OperationMode", ChatViewSettings::OptOut).toInt();
    settings["ShowHighlights"] = chatViewSettings.value("ShowHighlights", false);
    settings["ShowOwnMsgs"] = chatViewSettings.value("ShowOwnMsgs", true);
    settings["AlwaysOwn"] = chatViewSettings.value("AlwaysOwn", false);
    settings["Buffers"] = chatViewSettings.value("Buffers", QVariantList());
    settings["ShowBacklog"] = chatViewSettings.value("ShowBacklog", true);
    settings["IncludeRead"] = chatViewSettings.value("IncludeRead", false);
}

void ChatMonitorSettingsPage::save()
{
    ChatViewSettings chatViewSettings("ChatMonitor");
    // save operation mode
    chatViewSettings.setValue("OperationMode", ui.operationMode->currentIndex() + 1);
    chatViewSettings.setValue("ShowHighlights", ui.showHighlights->isChecked());
    chatViewSettings.setValue("ShowOwnMsgs", ui.showOwnMessages->isChecked());
    chatViewSettings.setValue("AlwaysOwn", ui.alwaysOwn->isChecked());
    chatViewSettings.setValue("ShowBacklog", ui.showBacklog->isChecked());
    chatViewSettings.setValue("IncludeRead", ui.includeRead->isChecked());

    // save list of active buffers
    QVariantList saveableBufferIdList;
    foreach (BufferId id, _configActive->bufferList()) {
        saveableBufferIdList << QVariant::fromValue(id);
    }

    chatViewSettings.setValue("Buffers", saveableBufferIdList);
    load();
    setChangedState(false);
}

void ChatMonitorSettingsPage::widgetHasChanged()
{
    bool changed = testHasChanged();
    if (changed != hasChanged())
        setChangedState(changed);
}

bool ChatMonitorSettingsPage::testHasChanged()
{
    if (settings["OperationMode"].toInt() != ui.operationMode->currentIndex() + 1)
        return true;
    if (settings["ShowHighlights"].toBool() != ui.showHighlights->isChecked())
        return true;
    if (settings["ShowOwnMsgs"].toBool() != ui.showOwnMessages->isChecked())
        return true;
    if (settings["AlwaysOwn"].toBool() != ui.alwaysOwn->isChecked())
        return true;
    if (settings["ShowBacklog"].toBool() != ui.showBacklog->isChecked())
        return true;
    if (settings["IncludeRead"].toBool() != ui.includeRead->isChecked())
        return true;

    if (_configActive->bufferList().count() != settings["Buffers"].toList().count())
        return true;

    QSet<BufferId> uiBufs = toQSet(_configActive->bufferList());
    QSet<BufferId> settingsBufs;
    foreach (QVariant v, settings["Buffers"].toList())
        settingsBufs << v.value<BufferId>();
    if (uiBufs != settingsBufs)
        return true;

    return false;
}

// TODO: - support drag 'n drop
//      - adding of complete networks(?)

/*
  toggleBuffers takes each a bufferView and its config for "input" and "output".
  Any selected item will be moved over from the input to the output bufferview.
*/
void ChatMonitorSettingsPage::toggleBuffers(BufferView* inView, BufferViewConfig* inCfg, BufferView* outView, BufferViewConfig* outCfg)
{
    // Fill QMap with selected items ordered by selection row
    QMap<int, QList<BufferId>> selectedBuffers;
    foreach (QModelIndex index, inView->selectionModel()->selectedIndexes()) {
        BufferId inBufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
        if (index.data(NetworkModel::ItemTypeRole) == NetworkModel::NetworkItemType) {
            // TODO:
            //  If item is a network: move over all children and skip other selected items of this node
        }
        else if (index.data(NetworkModel::ItemTypeRole) == NetworkModel::BufferItemType) {
            selectedBuffers[index.parent().row()] << inBufferId;
        }
    }

    // clear selection to be able to remove the bufferIds without errors
    inView->selectionModel()->clearSelection();

    /*
      Invalidate the BufferViewFilters' configs to get constant add/remove times
      even for huge lists.
      This can probably be removed whenever BufferViewConfig::bulkAdd or something
      like that is available.
    */
    qobject_cast<BufferViewFilter*>(outView->model())->setConfig(nullptr);
    qobject_cast<BufferViewFilter*>(inView->model())->setConfig(nullptr);

    // actually move the ids
    foreach (QList<BufferId> list, selectedBuffers) {
        foreach (BufferId buffer, list) {
            outCfg->addBuffer(buffer, 0);
            inCfg->removeBuffer(buffer);
        }
    }

    outView->setFilteredModel(Client::bufferModel(), outCfg);
    inView->setFilteredModel(Client::bufferModel(), inCfg);

    widgetHasChanged();
}

void ChatMonitorSettingsPage::on_activateBuffer_clicked()
{
    if (ui.availableBuffers->currentIndex().isValid() && ui.availableBuffers->selectionModel()->hasSelection()) {
        toggleBuffers(ui.availableBuffers, _configAvailable, ui.activeBuffers, _configActive);
        widgetHasChanged();
    }
}

void ChatMonitorSettingsPage::on_deactivateBuffer_clicked()
{
    if (ui.activeBuffers->currentIndex().isValid() && ui.activeBuffers->selectionModel()->hasSelection()) {
        toggleBuffers(ui.activeBuffers, _configActive, ui.availableBuffers, _configAvailable);
        widgetHasChanged();
    }
}

/*
  switchOperationMode gets called on combobox signal currentIndexChanged.
  modeIndex is the row id in combobox itemlist
*/
void ChatMonitorSettingsPage::switchOperationMode(int idx)
{
    auto mode = (ChatViewSettings::OperationMode)(idx + 1);
    if (mode == ChatViewSettings::OptIn) {
        ui.labelActiveBuffers->setText(tr("Show:"));
    }
    else if (mode == ChatViewSettings::OptOut) {
        ui.labelActiveBuffers->setText(tr("Ignore:"));
    }
    widgetHasChanged();
}

void ChatMonitorSettingsPage::setRequesterType(const QVariant& v)
{
    bool usingAsNeededRequester = (v.toInt() == BacklogRequester::AsNeeded);
    ui.showBacklogUnavailableDetails->setVisible(usingAsNeededRequester);
    if (usingAsNeededRequester) {
        ui.showBacklog->setText(tr("Show messages from backlog (not available)"));
    }
    else {
        ui.showBacklog->setText(tr("Show messages from backlog"));
    }
}

void ChatMonitorSettingsPage::on_showBacklogUnavailableDetails_clicked()
{
    // Explain that backlog fetching is disabled, so backlog messages won't show up
    //
    // Technically, backlog messages *will* show up once fetched, e.g. after clicking on a buffer.
    // This might be too trivial of a detail to warrant explaining, though.
    QMessageBox::information(this,
                             tr("Messages from backlog are not fetched"),
                             QString("<p>%1</p><p>%2</p>")
                                     .arg(tr("No initial backlog will be fetched when using the backlog request method of <i>%1</i>.")
                                             .arg(tr("Only fetch when needed").replace(" ", "&nbsp;")),
                                          tr("Configure this in the <i>%1</i> settings page.")
                                             .arg(tr("Backlog Fetching").replace(" ", "&nbsp;"))
                                     )
    );
    // Re-use translations of "Only fetch when needed" and "Backlog Fetching" as this is a
    // word-for-word reference, forcing all spaces to be non-breaking
}

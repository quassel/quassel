/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "bufferviewsettingspage.h"

#include <QMessageBox>

#include "client.h"
#include "network.h"
#include "bufferviewconfig.h"
#include "bufferviewfilter.h"
#include "bufferviewmanager.h"
#include "buffermodel.h"
#include "networkmodel.h"

BufferViewSettingsPage::BufferViewSettingsPage(QWidget *parent)
  : SettingsPage(tr("General"), tr("Buffer Views"), parent),
    _ignoreWidgetChanges(false)
{
  ui.setupUi(this);
  reset();

  ui.bufferViewList->setSortingEnabled(true);
  ui.settingsGroupBox->setEnabled(false);
  ui.bufferViewPreview->setEnabled(false);

  coreConnectionStateChanged(Client::isConnected());  // need a core connection!
  connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));
  connect(ui.bufferViewList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	  this, SLOT(bufferViewSelectionChanged(const QItemSelection &, const QItemSelection &)));

  connect(ui.onlyStatusBuffers, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.onlyChannelBuffers, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.onlyQueryBuffers, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.addNewBuffersAutomatically, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.sortAlphabetically, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.hideInactiveBuffers, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.networkSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.minimumActivitySelector, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
}

BufferViewSettingsPage::~BufferViewSettingsPage() {
  reset();
}

void BufferViewSettingsPage::reset() {
  ui.bufferViewList->clear();

  QHash<BufferViewConfig *, BufferViewConfig *>::iterator changedConfigIter = _changedBufferViews.begin();
  QHash<BufferViewConfig *, BufferViewConfig *>::iterator changedConfigIterEnd = _changedBufferViews.end();
  BufferViewConfig *config;
  while(changedConfigIter != changedConfigIterEnd) {
    config = changedConfigIter.value();
    changedConfigIter = _changedBufferViews.erase(changedConfigIter);
    config->deleteLater();
  }

  QList<BufferViewConfig *>::iterator newConfigIter = _newBufferViews.begin();
  QList<BufferViewConfig *>::iterator newConfigIterEnd = _newBufferViews.end();
  while(newConfigIter != newConfigIterEnd) {
    config = *newConfigIter;
    newConfigIter = _newBufferViews.erase(newConfigIter);
    config->deleteLater();
  }

  _deleteBufferViews.clear();
  
  setChangedState(false);
}

void BufferViewSettingsPage::load() {
  reset();

  if(!Client::bufferViewManager())
    return;

  const QList<BufferViewConfig *> bufferViewConfigs = Client::bufferViewManager()->bufferViewConfigs();
  foreach(BufferViewConfig *bufferViewConfig, bufferViewConfigs) {
    addBufferView(bufferViewConfig);
  }

  _ignoreWidgetChanges = true;
  // load network selector
  ui.networkSelector->clear();
  ui.networkSelector->addItem(tr("All"));
  ui.networkSelector->setItemData(0, qVariantFromValue<NetworkId>(NetworkId()));
  const Network *net;
  foreach(NetworkId netId, Client::networkIds()) {
    net = Client::network(netId);
    ui.networkSelector->addItem(net->networkName());
    ui.networkSelector->setItemData(ui.networkSelector->count() - 1, qVariantFromValue<NetworkId>(net->networkId()));
  }
  _ignoreWidgetChanges = false;
  
  ui.bufferViewList->setCurrentRow(0);
}

void BufferViewSettingsPage::save() {
  setEnabled(false);
  QVariantList newConfigs;
  QVariantList deleteConfigs;
  QVariantList changedConfigs;

  foreach(int bufferId, _deleteBufferViews) {
    deleteConfigs << bufferId;
  }
  _deleteBufferViews.clear();
  if(Client::bufferViewManager()) {
    Client::bufferViewManager()->requestDeleteBufferViews(deleteConfigs);
  }
  
  QHash<BufferViewConfig *, BufferViewConfig *>::iterator changedConfigIter = _changedBufferViews.begin();
  QHash<BufferViewConfig *, BufferViewConfig *>::iterator changedConfigIterEnd = _changedBufferViews.end();
  BufferViewConfig *config, *changedConfig;
  while(changedConfigIter != changedConfigIterEnd) {
    config = changedConfigIter.key();
    changedConfig = changedConfigIter.value();
    changedConfigIter = _changedBufferViews.erase(changedConfigIter);
    config->requestUpdate(changedConfig->toVariantMap());
    changedConfig->deleteLater();
  }

  QList<BufferViewConfig *>::iterator newConfigIter = _newBufferViews.begin();
  QList<BufferViewConfig *>::iterator newConfigIterEnd = _newBufferViews.end();
  while(newConfigIter != newConfigIterEnd) {
    config = *newConfigIter;
    newConfigIter = _newBufferViews.erase(newConfigIter);
    newConfigs << config->toVariantMap();
    config->deleteLater();
  }
  if(Client::bufferViewManager()) {
    Client::bufferViewManager()->requestCreateBufferViews(newConfigs);
  }

  load();
  setEnabled(true);
}

void BufferViewSettingsPage::coreConnectionStateChanged(bool state) {
  setEnabled(state);
  if(state) {
    load();
    connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigAdded(int)), this, SLOT(addBufferView(int)));
  } else {
    reset();
  }
}

void BufferViewSettingsPage::addBufferView(BufferViewConfig *config) {
  QListWidgetItem *item = new QListWidgetItem(config->bufferViewName(), ui.bufferViewList);
  item->setData(Qt::UserRole, qVariantFromValue<QObject *>(qobject_cast<QObject *>(config)));
  connect(config, SIGNAL(updatedRemotely()), this, SLOT(updateBufferView()));
  connect(config, SIGNAL(destroyed()), this, SLOT(bufferViewDeleted()));
}

void BufferViewSettingsPage::addBufferView(int bufferViewId) {
  Q_ASSERT(Client::bufferViewManager());
  addBufferView(Client::bufferViewManager()->bufferViewConfig(bufferViewId));
}

void BufferViewSettingsPage::bufferViewDeleted() {
  BufferViewConfig *config = static_cast<BufferViewConfig *>(sender());
  QObject *obj;
  for(int i = 0; i < ui.bufferViewList->count(); i++) {
    obj = ui.bufferViewList->item(i)->data(Qt::UserRole).value<QObject *>();
    if(config == static_cast<BufferViewConfig *>(obj)) {
      QListWidgetItem *item = ui.bufferViewList->takeItem(i);
      delete item;
      return;
    }
  }
}

void BufferViewSettingsPage::newBufferView(const QString &bufferViewName) {
  // id's of newly created bufferviews are negative (-1, -2... -n)
  int fakeId = -1 * (_newBufferViews.count() + 1);
  BufferViewConfig *config = new BufferViewConfig(fakeId);
  config->setBufferViewName(bufferViewName);

  QList<BufferId> bufferIds;
  if(config->addNewBuffersAutomatically()) {
    bufferIds = Client::networkModel()->allBufferIds();
    if(config->sortAlphabetically())
      qSort(bufferIds.begin(), bufferIds.end(), bufferIdLessThan);
  }
  config->initSetBufferList(bufferIds);

  _newBufferViews << config;
  addBufferView(config);
  ui.bufferViewList->setCurrentRow(listPos(config));
}
      
int BufferViewSettingsPage::listPos(BufferViewConfig *config) {
  QObject *obj;
  for(int i = 0; i < ui.bufferViewList->count(); i++) {
    obj = ui.bufferViewList->item(i)->data(Qt::UserRole).value<QObject *>();
    if(config == qobject_cast<BufferViewConfig *>(obj))
      return i;
  }
  return -1;
}

BufferViewConfig *BufferViewSettingsPage::bufferView(int listPos) {
  if(listPos < ui.bufferViewList->count() && listPos >= 0) {
    QObject *obj = ui.bufferViewList->item(listPos)->data(Qt::UserRole).value<QObject *>();
    return qobject_cast<BufferViewConfig *>(obj);
  } else {
    return 0;
  }
}

void BufferViewSettingsPage::updateBufferView() {
  BufferViewConfig *config = qobject_cast<BufferViewConfig *>(sender());
  if(!config)
    return;

  int itemPos = listPos(config);
  if(itemPos == -1) {
    qWarning() << "BufferViewSettingsPage::updateBufferView(): view is unknown:" << config->bufferViewId();
    return;
  }
  ui.bufferViewList->item(itemPos)->setText(config->bufferViewName());
  if(itemPos == ui.bufferViewList->currentRow())
    loadConfig(config);
}

void BufferViewSettingsPage::on_addBufferView_clicked() {
  if(!Client::bufferViewManager())
    return;
  
  QStringList existing;
  foreach(BufferViewConfig *bufferConfig, Client::bufferViewManager()->bufferViewConfigs()) {
    existing << bufferConfig->bufferViewName();
  }

  BufferViewEditDlg dlg(QString(), existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    newBufferView(dlg.bufferViewName());
    changed();
  }
}

void BufferViewSettingsPage::on_renameBufferView_clicked() {
  if(ui.bufferViewList->selectedItems().isEmpty())
    return;

  if(!Client::bufferViewManager())
    return;
  
  BufferViewConfig *config = bufferView(ui.bufferViewList->currentRow());
  if(!config)
    return;

  QStringList existing;
  foreach(BufferViewConfig *bufferConfig, Client::bufferViewManager()->bufferViewConfigs()) {
    existing << bufferConfig->bufferViewName();
  }

  BufferViewEditDlg dlg(config->bufferViewName(), existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    BufferViewConfig *changedConfig = cloneConfig(config);
    changedConfig->setBufferViewName(dlg.bufferViewName());
    ui.bufferViewList->item(listPos(config))->setText(dlg.bufferViewName());
    changed();
  }
}

void BufferViewSettingsPage::on_deleteBufferView_clicked() {
  if(ui.bufferViewList->selectedItems().isEmpty())
    return;

  QListWidgetItem *currentItem = ui.bufferViewList->item(ui.bufferViewList->currentRow());
  QString viewName = currentItem->text();
  int viewId = bufferView(ui.bufferViewList->currentRow())->bufferViewId();
  int ret = QMessageBox::question(this, tr("Delete Buffer View?"),
                                    tr("Do you really want to delete the buffer view \"%1\"?").arg(viewName),
                                    QMessageBox::Yes|QMessageBox::No, QMessageBox::No);

  if(ret == QMessageBox::Yes) {
    ui.bufferViewList->removeItemWidget(currentItem);
    delete currentItem;
    if(viewId >= 0)
      _deleteBufferViews << viewId;
    changed();
  }
}

void BufferViewSettingsPage::bufferViewSelectionChanged(const QItemSelection &current, const QItemSelection &previous) {
  Q_UNUSED(previous)

  if(!current.isEmpty()) {
    ui.settingsGroupBox->setEnabled(true);
    ui.bufferViewPreview->setEnabled(true);

    loadConfig(configForDisplay(bufferView(ui.bufferViewList->currentRow())));
  } else {
    ui.settingsGroupBox->setEnabled(false);
    ui.bufferViewPreview->setEnabled(false);
  }
}

void BufferViewSettingsPage::loadConfig(BufferViewConfig *config) {
  if(!config)
    return;
  
  _ignoreWidgetChanges = true;
  ui.onlyStatusBuffers->setChecked(BufferInfo::StatusBuffer & config->allowedBufferTypes());
  ui.onlyChannelBuffers->setChecked(BufferInfo::ChannelBuffer & config->allowedBufferTypes());
  ui.onlyQueryBuffers->setChecked(BufferInfo::QueryBuffer & config->allowedBufferTypes());
  ui.addNewBuffersAutomatically->setChecked(config->addNewBuffersAutomatically());
  ui.sortAlphabetically->setChecked(config->sortAlphabetically());
  ui.hideInactiveBuffers->setChecked(config->hideInactiveBuffers());

  int networkIndex = 0;
  for(int i = 0; i < ui.networkSelector->count(); i++) {
    if(ui.networkSelector->itemData(i).value<NetworkId>() == config->networkId()) {
      networkIndex = i;
      break;
    }
  }
  ui.networkSelector->setCurrentIndex(networkIndex);

  int activityIndex = 0;
  int minimumActivity = config->minimumActivity();
  while(minimumActivity) {
    activityIndex++;
    minimumActivity = minimumActivity >> 1;
  }
  ui.minimumActivitySelector->setCurrentIndex(activityIndex);

  ui.bufferViewPreview->setFilteredModel(Client::bufferModel(), config);
  
  _ignoreWidgetChanges = false;
}

void BufferViewSettingsPage::saveConfig(BufferViewConfig *config) {
  if(!config)
    return;
  
  int allowedBufferTypes = 0;
  if(ui.onlyStatusBuffers->isChecked())
    allowedBufferTypes |= BufferInfo::StatusBuffer;
  if(ui.onlyChannelBuffers->isChecked())
    allowedBufferTypes |= BufferInfo::ChannelBuffer;
  if(ui.onlyQueryBuffers->isChecked())
    allowedBufferTypes |= BufferInfo::QueryBuffer;
  config->setAllowedBufferTypes(allowedBufferTypes);

  config->setAddNewBuffersAutomatically(ui.addNewBuffersAutomatically->isChecked());
  config->setSortAlphabetically(ui.sortAlphabetically->isChecked());
  config->setHideInactiveBuffers(ui.hideInactiveBuffers->isChecked());
  config->setNetworkId(ui.networkSelector->itemData(ui.networkSelector->currentIndex()).value<NetworkId>());

  int minimumActivity = 0;
  if(ui.minimumActivitySelector->currentIndex() > 0)
    minimumActivity = 1 << (ui.minimumActivitySelector->currentIndex() - 1);
  config->setMinimumActivity(minimumActivity);

  if(_newBufferViews.contains(config)) {
    QList<BufferId> bufferIds;
    if(config->addNewBuffersAutomatically()) {
      bufferIds = Client::networkModel()->allBufferIds();
      if(config->sortAlphabetically())
	qSort(bufferIds.begin(), bufferIds.end(), bufferIdLessThan);
    }
    config->initSetBufferList(bufferIds);
  }
}

void BufferViewSettingsPage::widgetHasChanged() {
  if(_ignoreWidgetChanges)
    return;
  setChangedState(testHasChanged());
}

bool BufferViewSettingsPage::testHasChanged() {
  saveConfig(cloneConfig(bufferView(ui.bufferViewList->currentRow())));

  if(!_newBufferViews.isEmpty())
    return true;

  bool changed = false;
  QHash<BufferViewConfig *, BufferViewConfig *>::iterator iter = _changedBufferViews.begin();
  QHash<BufferViewConfig *, BufferViewConfig *>::iterator iterEnd = _changedBufferViews.end();
  while(iter != iterEnd) {
    if(&(iter.key()) == &(iter.value())) {
      iter.value()->deleteLater();
      _changedBufferViews.erase(iter);
    } else {
      changed = true;
      iter++;
    }
  }
  return changed;
}

BufferViewConfig *BufferViewSettingsPage::cloneConfig(BufferViewConfig *config) {
  if(!config || config->bufferViewId() < 0)
    return config;
  
  if(_changedBufferViews.contains(config))
    return _changedBufferViews[config];

  BufferViewConfig *changedConfig = new BufferViewConfig(-1, this);
  changedConfig->fromVariantMap(config->toVariantMap());
  changedConfig->setInitialized();
  _changedBufferViews[config] = changedConfig;
  connect(config, SIGNAL(bufferAdded(const BufferId &, int)), changedConfig, SLOT(addBuffer(const BufferId &, int)));
  connect(config, SIGNAL(bufferMoved(const BufferId &, int)), changedConfig, SLOT(moveBuffer(const BufferId &, int)));
  connect(config, SIGNAL(bufferRemoved(const BufferId &)), changedConfig, SLOT(removeBuffer(const BufferId &)));
  connect(config, SIGNAL(addBufferRequested(const BufferId &, int)), changedConfig, SLOT(addBuffer(const BufferId &, int)));
  connect(config, SIGNAL(moveBufferRequested(const BufferId &, int)), changedConfig, SLOT(moveBuffer(const BufferId &, int)));
  connect(config, SIGNAL(removeBufferRequested(const BufferId &)), changedConfig, SLOT(removeBuffer(const BufferId &)));

  // if this is the currently displayed view we have to change the config of the preview filter
  BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(ui.bufferViewPreview->model());
  if(filter && filter->config() == config)
    filter->setConfig(changedConfig);
  ui.bufferViewPreview->setConfig(changedConfig);

  return changedConfig;
}

BufferViewConfig *BufferViewSettingsPage::configForDisplay(BufferViewConfig *config) {
  if(_changedBufferViews.contains(config))
    return _changedBufferViews[config];
  else
    return config;
}


/**************************************************************************
 * BufferViewEditDlg
 *************************************************************************/
BufferViewEditDlg::BufferViewEditDlg(const QString &old, const QStringList &exist, QWidget *parent) : QDialog(parent), existing(exist) {
  ui.setupUi(this);

  if(old.isEmpty()) {
    // new buffer
    setWindowTitle(tr("Add Buffer View"));
    on_bufferViewEdit_textChanged(""); // disable ok button
  } else {
    ui.bufferViewEdit->setText(old);
  }
}


void BufferViewEditDlg::on_bufferViewEdit_textChanged(const QString &text) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty() || existing.contains(text));
}


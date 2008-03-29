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

#include "client.h"
#include "bufferviewmanager.h"
#include "bufferviewconfig.h"

BufferViewSettingsPage::BufferViewSettingsPage(QWidget *parent)
  : SettingsPage(tr("General"), tr("Buffer Views"), parent)
{
  ui.setupUi(this);
  reset();
  setEnabled(Client::isConnected());  // need a core connection!
  connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));
}

BufferViewSettingsPage::~BufferViewSettingsPage() {
  reset();
}

void BufferViewSettingsPage::reset() {
  ui.bufferViewList->clear();
  _viewToListPos.clear();
  _listPosToView.clear();

  QHash<BufferViewConfig *, BufferViewConfig *>::iterator changedConfigIter = _changedBufferViews.begin();
  QHash<BufferViewConfig *, BufferViewConfig *>::iterator changedConfigIterEnd = _changedBufferViews.end();
  BufferViewConfig *config;
  while(changedConfigIter != changedConfigIterEnd) {
    config = (*changedConfigIter);
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
}

void BufferViewSettingsPage::save() {
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
  _viewToListPos[config->bufferViewId()] = ui.bufferViewList->count();
  _listPosToView[ui.bufferViewList->count()] = config->bufferViewId();
  ui.bufferViewList->addItem(config->bufferViewName());
  connect(config, SIGNAL(updatedRemotely()), this, SLOT(updateBufferView()));
}

void BufferViewSettingsPage::addBufferView(int bufferViewId) {
  Q_ASSERT(Client::bufferViewManager());
  addBufferView(Client::bufferViewManager()->bufferViewConfig(bufferViewId));
}

void BufferViewSettingsPage::newBufferView(const QString &bufferViewName) {
  // id's of newly created bufferviews are negative (-1, -2... -n)
  int fakeId = -1 * (_newBufferViews.count() + 1);
  BufferViewConfig *config = new BufferViewConfig(fakeId);
  config->setBufferViewName(bufferViewName);
  _newBufferViews << config;
  addBufferView(config);
}
      
int BufferViewSettingsPage::listPos(BufferViewConfig *config) {
  if(_viewToListPos.contains(config->bufferViewId()))
    return _viewToListPos[config->bufferViewId()];
  else
    return -1;
}

int BufferViewSettingsPage::bufferViewId(int listPos) {
  if(_listPosToView.contains(listPos))
    return _listPosToView[listPos];
  else
    return -1;
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
  
  BufferViewConfig *config = Client::bufferViewManager()->bufferViewConfig(bufferViewId(ui.bufferViewList->currentRow()));
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
    changed();
  }
}

BufferViewConfig *BufferViewSettingsPage::cloneConfig(BufferViewConfig *config) {
  if(_changedBufferViews.contains(config))
    return _changedBufferViews[config];

  BufferViewConfig *changedConfig = new BufferViewConfig(-1, this);
  changedConfig->fromVariantMap(config->toVariantMap());
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


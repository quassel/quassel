/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "fontssettingspage.h"

#include "qtui.h"

#include <QFontDialog>

FontsSettingsPage::FontsSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("Fonts"), parent) {

  ui.setupUi(this);
  mapper = new QSignalMapper(this);
  connect(ui.chooseGeneral, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseTopic, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseNickList, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseBufferView, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseChatMessages, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseNicks, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseTimestamp, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.chooseGeneral, ui.demoGeneral);
  mapper->setMapping(ui.chooseTopic, ui.demoTopic);
  mapper->setMapping(ui.chooseNickList, ui.demoNickList);
  mapper->setMapping(ui.chooseBufferView, ui.demoBufferView);
  mapper->setMapping(ui.chooseChatMessages, ui.demoChatMessages);
  mapper->setMapping(ui.chooseNicks, ui.demoNicks);
  mapper->setMapping(ui.chooseTimestamp, ui.demoTimestamp);

  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseFont(QWidget *)));

  load();

}

bool FontsSettingsPage::hasChanged() const {

  return false;
}

void FontsSettingsPage::defaults() {
  load(Settings::Default);

}

void FontsSettingsPage::load() {
  load(Settings::Custom);
  changeState(false);
}

void FontsSettingsPage::load(Settings::Mode mode) {
  QTextCharFormat chatFormat = QtUi::style()->format(UiStyle::None, mode);
  setFont(ui.demoChatMessages, chatFormat.font());
  QTextCharFormat nicksFormat = QtUi::style()->format(UiStyle::Sender, mode);
  if(nicksFormat.hasProperty(QTextFormat::FontFamily)) {
    setFont(ui.demoNicks, nicksFormat.font());
    ui.checkNicks->setChecked(true);
  } else {
    setFont(ui.demoNicks, chatFormat.font());
    ui.checkNicks->setChecked(false);
  }
  QTextCharFormat timestampFormat = QtUi::style()->format(UiStyle::Timestamp, mode);
  if(timestampFormat.hasProperty(QTextFormat::FontFamily)) {
    setFont(ui.demoTimestamp, timestampFormat.font());
    ui.checkTimestamp->setChecked(true);
  } else {
    setFont(ui.demoTimestamp, chatFormat.font());
    ui.checkTimestamp->setChecked(false);
  }

}

void FontsSettingsPage::save() {
  QTextCharFormat chatFormat = QtUi::style()->format(UiStyle::None);
  chatFormat.setFont(ui.demoChatMessages->font());
  QtUi::style()->setFormat(UiStyle::None, chatFormat, Settings::Custom);

  //FIXME: actually remove font properties from the formats
  QTextCharFormat nicksFormat = QtUi::style()->format(UiStyle::Sender);
  if(ui.checkNicks->checkState() == Qt::Checked) nicksFormat.setFont(ui.demoNicks->font());
  else nicksFormat.setFont(chatFormat.font());
  QtUi::style()->setFormat(UiStyle::Sender, nicksFormat, Settings::Custom);

  QTextCharFormat timestampFormat = QtUi::style()->format(UiStyle::Timestamp);
  if(ui.checkTimestamp->checkState() == Qt::Checked) timestampFormat.setFont(ui.demoTimestamp->font());
  else timestampFormat.setFont(chatFormat.font());
  QtUi::style()->setFormat(UiStyle::Timestamp, timestampFormat, Settings::Custom);

  changeState(false);
}

void FontsSettingsPage::setFont(QLabel *label, const QFont &font) {
  QFontInfo fontInfo(font);
  label->setFont(font);
  label->setText(QString("%1 %2").arg(fontInfo.family()).arg(fontInfo.pointSize()));
}

void FontsSettingsPage::chooseFont(QWidget *widget) {
  QLabel *label = qobject_cast<QLabel *>(widget);
  Q_ASSERT(label);
  bool ok;
  QFont font = QFontDialog::getFont(&ok, label->font());
  if(ok) {
    setFont(label, font);
  }
}

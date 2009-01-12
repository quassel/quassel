/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include "qtuisettings.h"
#include "qtuistyle.h"

#include <QFontDialog>

FontsSettingsPage::FontsSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("Fonts"), parent) {

  ui.setupUi(this);
  mapper = new QSignalMapper(this);
  connect(ui.chooseGeneral, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseTopic, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseBufferView, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseNickList, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseInputLine, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseChatMessages, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseNicks, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseTimestamp, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.chooseGeneral, ui.demoGeneral);
  mapper->setMapping(ui.chooseTopic, ui.demoTopic);
  mapper->setMapping(ui.chooseBufferView, ui.demoBufferView);
  mapper->setMapping(ui.chooseNickList, ui.demoNickList);
  mapper->setMapping(ui.chooseInputLine, ui.demoInputLine);
  mapper->setMapping(ui.chooseChatMessages, ui.demoChatMessages);
  mapper->setMapping(ui.chooseNicks, ui.demoNicks);
  mapper->setMapping(ui.chooseTimestamp, ui.demoTimestamp);

  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseFont(QWidget *)));

  //connect(ui.customAppFonts, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.checkTopic, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.checkBufferView, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.checkNickList, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.checkInputLine, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.checkNicks, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.checkTimestamp, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));

  load();

}

bool FontsSettingsPage::hasDefaults() const {
  return true;
}

void FontsSettingsPage::defaults() {
  load(Settings::Default);
  widgetHasChanged();
}

void FontsSettingsPage::load() {
  load(Settings::Custom);
  setChangedState(false);
}

void FontsSettingsPage::load(Settings::Mode mode) {
  QtUiSettings s;
  bool useInputLineFont = s.value("UseInputLineFont", QVariant(false)).toBool();
  QFont inputLineFont;
  if(useInputLineFont) {
    ui.checkInputLine->setChecked(true);
    inputLineFont = s.value("InputLineFont").value<QFont>();
  } else {
    inputLineFont = qApp->font();
  }
  initLabel(ui.demoInputLine, inputLineFont);

  QTextCharFormat chatFormat = QtUi::style()->format(UiStyle::None, mode);
  initLabel(ui.demoChatMessages, chatFormat.font());
  QTextCharFormat nicksFormat = QtUi::style()->format(UiStyle::Sender, mode);
  if(nicksFormat.hasProperty(QTextFormat::FontFamily)) {
    initLabel(ui.demoNicks, nicksFormat.font());
    ui.checkNicks->setChecked(true);
  } else {
    initLabel(ui.demoNicks, chatFormat.font());
    ui.checkNicks->setChecked(false);
  }

  QTextCharFormat timestampFormat = QtUi::style()->format(UiStyle::Timestamp, mode);
  if(timestampFormat.hasProperty(QTextFormat::FontFamily)) {
    initLabel(ui.demoTimestamp, timestampFormat.font());
    ui.checkTimestamp->setChecked(true);
  } else {
    initLabel(ui.demoTimestamp, chatFormat.font());
    ui.checkTimestamp->setChecked(false);
  }

  setChangedState(false);
}

void FontsSettingsPage::save() {
  QtUiSettings s;
  s.setValue("UseInputLineFont", (ui.checkInputLine->checkState() == Qt::Checked));
  s.setValue("InputLineFont", ui.demoInputLine->font());

  QTextCharFormat chatFormat = QtUi::style()->format(UiStyle::None);
  chatFormat.setFont(ui.demoChatMessages->font());
  QtUi::style()->setFormat(UiStyle::None, chatFormat, Settings::Custom);

  QTextCharFormat nicksFormat = QtUi::style()->format(UiStyle::Sender);
  if(ui.checkNicks->checkState() == Qt::Checked)
    nicksFormat.setFont(ui.demoNicks->font());
  else
    clearFontFromFormat(nicksFormat);
  QtUi::style()->setFormat(UiStyle::Sender, nicksFormat, Settings::Custom);

  QTextCharFormat timestampFormat = QtUi::style()->format(UiStyle::Timestamp);
  if(ui.checkTimestamp->checkState() == Qt::Checked)
    timestampFormat.setFont(ui.demoTimestamp->font());
  else
    clearFontFromFormat(timestampFormat);
  QtUi::style()->setFormat(UiStyle::Timestamp, timestampFormat, Settings::Custom);

  setChangedState(false);
}

void FontsSettingsPage::widgetHasChanged() {
  if(!hasChanged()) setChangedState(true);
}

void FontsSettingsPage::initLabel(QLabel *label, const QFont &font) {
  setFont(label, font);
}

void FontsSettingsPage::setFont(QLabel *label, const QFont &font) {
  label->setFont(font);
  label->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
  widgetHasChanged();
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

void FontsSettingsPage::clearFontFromFormat(QTextCharFormat &fmt) {
  fmt.clearProperty(QTextFormat::FontFamily);
  fmt.clearProperty(QTextFormat::FontPointSize);
  fmt.clearProperty(QTextFormat::FontPixelSize);
  fmt.clearProperty(QTextFormat::FontWeight);
  fmt.clearProperty(QTextFormat::FontItalic);
  fmt.clearProperty(QTextFormat::TextUnderlineStyle);
  fmt.clearProperty(QTextFormat::FontOverline);
  fmt.clearProperty(QTextFormat::FontStrikeOut);
  fmt.clearProperty(QTextFormat::FontFixedPitch);
  fmt.clearProperty(QTextFormat::FontCapitalization);
  fmt.clearProperty(QTextFormat::FontWordSpacing);
  fmt.clearProperty(QTextFormat::FontLetterSpacing);
}

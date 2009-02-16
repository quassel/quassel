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

#include "appearancesettingspage.h"

#include "buffersettings.h"
#include "chatviewsettings.h"
#include "qtui.h"
#include "qtuisettings.h"
#include "qtuistyle.h"
#include "util.h"

#include <QDir>
#include <QFontDialog>
#include <QSignalMapper>
#include <QStyleFactory>

AppearanceSettingsPage::AppearanceSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), QString(), parent)
{
  ui.setupUi(this);
  initStyleComboBox();
  initLanguageComboBox();

#ifndef HAVE_WEBKIT
  ui.showWebPreview->hide();
  ui.showWebPreview->setEnabled(false);
#endif

  foreach(QComboBox *comboBox, findChildren<QComboBox *>()) {
    connect(comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(widgetHasChanged()));
  }
  foreach(QCheckBox *checkBox, findChildren<QCheckBox *>()) {
    connect(checkBox, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  }

  mapper = new QSignalMapper(this);
  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseFont(QWidget *)));

  connect(ui.chooseChatView, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseBufferView, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.chooseInputLine, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.chooseChatView, ui.demoChatView);
  mapper->setMapping(ui.chooseBufferView, ui.demoBufferView);
  mapper->setMapping(ui.chooseInputLine, ui.demoInputLine);
}

void AppearanceSettingsPage::initStyleComboBox() {
  QStringList styleList = QStyleFactory::keys();
  ui.styleComboBox->addItem(tr("<System Default>"));
  foreach(QString style, styleList) {
    ui.styleComboBox->addItem(style);
  }
}

void AppearanceSettingsPage::initLanguageComboBox() {
  QDir i18nDir(Quassel::translationDirPath(), "quassel_*.qm");

  foreach(QString translationFile, i18nDir.entryList()) {
    QString localeName(translationFile.mid(8));
    localeName.chop(3);
    QLocale locale(localeName);
    _locales << locale;
    ui.languageComboBox->addItem(QLocale::languageToString(locale.language()));
  }
}

void AppearanceSettingsPage::defaults() {
  ui.styleComboBox->setCurrentIndex(0);

  loadFonts(Settings::Default);
  _fontsChanged = true;
  widgetHasChanged();
}

void AppearanceSettingsPage::load() {
  QtUiSettings uiSettings;

  // Gui Style
  settings["Style"] = uiSettings.value("Style", QString(""));
  if(settings["Style"].toString() == "") {
    ui.styleComboBox->setCurrentIndex(0);
  } else {
    ui.styleComboBox->setCurrentIndex(ui.styleComboBox->findText(settings["Style"].toString(), Qt::MatchExactly));
    QApplication::setStyle(settings["Style"].toString());
  }

  // Language
  QLocale locale = uiSettings.value("Locale", QLocale::system()).value<QLocale>();
  if(locale == QLocale::system())
    ui.languageComboBox->setCurrentIndex(0);
  else if(locale.language() == QLocale::C)
    ui.languageComboBox->setCurrentIndex(1);
  else
    ui.languageComboBox->setCurrentIndex(ui.languageComboBox->findText(QLocale::languageToString(locale.language()), Qt::MatchExactly));
  Quassel::loadTranslation(selectedLocale());

  ChatViewSettings chatViewSettings;
  SettingsPage::load(ui.showWebPreview, chatViewSettings.showWebPreview());

  BufferSettings bufferSettings;
  SettingsPage::load(ui.showUserStateIcons, bufferSettings.showUserStateIcons());

  loadFonts(Settings::Custom);

  setChangedState(false);
}

void AppearanceSettingsPage::loadFonts(Settings::Mode mode) {
  QtUiStyleSettings s("Fonts");

  QFont inputLineFont;
  if(mode == Settings::Custom)
    inputLineFont = s.value("InputLine", QFont()).value<QFont>();
  setFont(ui.demoInputLine, inputLineFont);

  QFont bufferViewFont;
  if(mode == Settings::Custom)
    bufferViewFont = s.value("BufferView", QFont()).value<QFont>();
  setFont(ui.demoBufferView, bufferViewFont);

  QTextCharFormat chatFormat = QtUi::style()->format(UiStyle::None, mode);
  setFont(ui.demoChatView, chatFormat.font());

  _fontsChanged = false;
}

void AppearanceSettingsPage::save() {
  QtUiSettings uiSettings;

  if(ui.styleComboBox->currentIndex() < 1) {
    uiSettings.setValue("Style", QString(""));
  } else {
    uiSettings.setValue("Style", ui.styleComboBox->currentText());
  }

  if(ui.languageComboBox->currentIndex() == 0) {
    uiSettings.remove("Locale"); // force the default (QLocale::system())
  } else {
    uiSettings.setValue("Locale", selectedLocale());
  }

  ChatViewSettings chatViewSettings;
  chatViewSettings.enableWebPreview(ui.showWebPreview->isChecked());

  BufferSettings bufferSettings;
  bufferSettings.enableUserStateIcons(ui.showUserStateIcons->isChecked());

  // Fonts
  QtUiStyleSettings fontSettings("Fonts");
  if(ui.demoInputLine->font() != QApplication::font())
    fontSettings.setValue("InputLine", ui.demoInputLine->font());
  else
    fontSettings.setValue("InputLine", "");

  if(ui.demoBufferView->font() != QApplication::font())
    fontSettings.setValue("BufferView", ui.demoBufferView->font());
  else
    fontSettings.setValue("BufferView", "");

  QTextCharFormat chatFormat = QtUi::style()->format(UiStyle::None);
  chatFormat.setFont(ui.demoChatView->font());
  QtUi::style()->setFormat(UiStyle::None, chatFormat, Settings::Custom);

  _fontsChanged = false;

  load();
  setChangedState(false);
}

QLocale AppearanceSettingsPage::selectedLocale() const {
  QLocale locale;
  int index = ui.languageComboBox->currentIndex();
  if(index == 0)
    locale = QLocale::system();
  else if(index == 1)
    locale = QLocale::c();
  else if(index > 1)
    locale = _locales[index - 2];

  return locale;
}

void AppearanceSettingsPage::setFont(QLabel *label, const QFont &font_) {
  QFont font = font_;
  if(font.family().isEmpty())
    font = QApplication::font();
  label->setFont(font);
  label->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
  widgetHasChanged();
}

void AppearanceSettingsPage::chooseFont(QWidget *widget) {
  QLabel *label = qobject_cast<QLabel *>(widget);
  Q_ASSERT(label);
  bool ok;
  QFont font = QFontDialog::getFont(&ok, label->font());
  if(ok) {
    setFont(label, font);
    _fontsChanged = true;
  }
}

void AppearanceSettingsPage::widgetHasChanged() {
  setChangedState(testHasChanged());
}

bool AppearanceSettingsPage::testHasChanged() {
  if(_fontsChanged) return true; // comparisons are nasty for now

  if(settings["Style"].toString() != ui.styleComboBox->currentText()) return true;
  if(selectedLocale() != QLocale()) return true; // QLocale() returns the default locale (manipulated via loadTranslation())

  if(SettingsPage::hasChanged(ui.showWebPreview)) return true;
  if(SettingsPage::hasChanged(ui.showUserStateIcons)) return true;

  return false;
}

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

#include "appearancesettingspage.h"

#include "buffersettings.h"
#include "chatviewsettings.h"
#include "qtui.h"
#include "qtuisettings.h"
#include "util.h"

#include <QDir>
#include <QStyleFactory>

AppearanceSettingsPage::AppearanceSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("General"), parent) {
  ui.setupUi(this);
  initStyleComboBox();
  initLanguageComboBox();

  foreach(QComboBox *comboBox, findChildren<QComboBox *>()) {
    connect(comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(widgetHasChanged()));
  }
  foreach(QCheckBox *checkBox, findChildren<QCheckBox *>()) {
    connect(checkBox, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  }
}

void AppearanceSettingsPage::initStyleComboBox() {
  QStringList styleList = QStyleFactory::keys();
  ui.styleComboBox->addItem(tr("<System Default>"));
  foreach(QString style, styleList) {
    ui.styleComboBox->addItem(style);
  }
}

void AppearanceSettingsPage::initLanguageComboBox() {
  QDir i18nDir(":/i18n", "quassel_*.qm");

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
  loadTranslation(selectedLocale());

  ChatViewSettings chatViewSettings;
  SettingsPage::load(ui.showWebPreview, chatViewSettings.showWebPreview());

  BufferSettings bufferSettings;
  SettingsPage::load(ui.showUserStateIcons, bufferSettings.showUserStateIcons());

  setChangedState(false);
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

void AppearanceSettingsPage::widgetHasChanged() {
  setChangedState(testHasChanged());
}

bool AppearanceSettingsPage::testHasChanged() {
  if(settings["Style"].toString() != ui.styleComboBox->currentText()) return true;
  if(selectedLocale() != QLocale()) return true; // QLocale() returns the default locale (manipulated via loadTranslation())

  if(SettingsPage::hasChanged(ui.showWebPreview)) return true;
  if(SettingsPage::hasChanged(ui.showUserStateIcons)) return true;

  return false;
}





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

#include "qtui.h"
#include "qtuisettings.h"
#include "qtuistyle.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QStyleFactory>
#include <QFile>
#include <QDir>

AppearanceSettingsPage::AppearanceSettingsPage(QWidget *parent)
  : SettingsPage(tr("Interface"), QString(), parent)
{
  ui.setupUi(this);
  initAutoWidgets();
  initStyleComboBox();
  initLanguageComboBox();

  foreach(QComboBox *comboBox, findChildren<QComboBox *>()) {
    connect(comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(widgetHasChanged()));
  }
  foreach(QCheckBox *checkBox, findChildren<QCheckBox *>()) {
    connect(checkBox, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  }

  connect(ui.chooseStyleSheet, SIGNAL(clicked()), SLOT(chooseStyleSheet()));
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

  SettingsPage::defaults();
  widgetHasChanged();
}

void AppearanceSettingsPage::load() {
  QtUiSettings uiSettings;

  // Gui Style
  QString style = uiSettings.value("Style", QString("")).toString();
  if(style.isEmpty()) {
    ui.styleComboBox->setCurrentIndex(0);
  } else {
    ui.styleComboBox->setCurrentIndex(ui.styleComboBox->findText(style, Qt::MatchExactly));
    QApplication::setStyle(style);
  }
  ui.styleComboBox->setProperty("storedValue", ui.styleComboBox->currentIndex());

  // Language
  QLocale locale = uiSettings.value("Locale", QLocale::system()).value<QLocale>();
  if(locale == QLocale::system())
    ui.languageComboBox->setCurrentIndex(0);
  else if(locale.language() == QLocale::C)
    ui.languageComboBox->setCurrentIndex(1);
  else
    ui.languageComboBox->setCurrentIndex(ui.languageComboBox->findText(QLocale::languageToString(locale.language()), Qt::MatchExactly));
  ui.languageComboBox->setProperty("storedValue", ui.languageComboBox->currentIndex());
  Quassel::loadTranslation(selectedLocale());

  SettingsPage::load();
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

  bool needsStyleReload =
        ui.useCustomStyleSheet->isChecked() != ui.useCustomStyleSheet->property("storedValue").toBool()
    || (ui.useCustomStyleSheet->isChecked() && ui.customStyleSheetPath->text() != ui.customStyleSheetPath->property("storedValue").toString());

  SettingsPage::save();
  setChangedState(false);
  if(needsStyleReload)
    QtUi::style()->reload();
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

void AppearanceSettingsPage::chooseStyleSheet() {
  QString dir = ui.customStyleSheetPath->property("storedValue").toString();
  if(!dir.isEmpty() && QFile(dir).exists())
    dir = QDir(dir).absolutePath();
  else
    dir = QDir(Quassel::findDataFilePath("default.qss")).absolutePath();

  QString name = QFileDialog::getOpenFileName(this, tr("Please choose a stylesheet file"), dir, "*.qss");
  if(!name.isEmpty())
    ui.customStyleSheetPath->setText(name);
}

void AppearanceSettingsPage::widgetHasChanged() {
  setChangedState(testHasChanged());
}

bool AppearanceSettingsPage::testHasChanged() {
  if(ui.styleComboBox->currentIndex() != ui.styleComboBox->property("storedValue").toInt()) return true;

  if(selectedLocale() != QLocale()) return true; // QLocale() returns the default locale (manipulated via loadTranslation())

  return false;
}

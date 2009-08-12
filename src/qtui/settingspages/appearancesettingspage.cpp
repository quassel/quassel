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

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QSignalMapper>
#include <QStyleFactory>

AppearanceSettingsPage::AppearanceSettingsPage(QWidget *parent)
  : SettingsPage(tr("Interface"), QString(), parent),
  _fontsChanged(false)
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

  mapper = new QSignalMapper(this);
  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseFont(QWidget *)));

  connect(ui.chooseInputLine, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.chooseInputLine, ui.demoInputLine);

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

  loadFonts(Settings::Default);
  _fontsChanged = true;

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

  loadFonts(Settings::Custom);

  SettingsPage::load();
  setChangedState(false);
}

void AppearanceSettingsPage::loadFonts(Settings::Mode mode) {
  QtUiStyleSettings s("Fonts");

  QFont inputLineFont;
  if(mode == Settings::Custom)
    inputLineFont = s.value("InputLine", QFont()).value<QFont>();
  setFont(ui.demoInputLine, inputLineFont);

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

  // Fonts
  QtUiStyleSettings fontSettings("Fonts");
  if(ui.demoInputLine->font() != QApplication::font())
    fontSettings.setValue("InputLine", ui.demoInputLine->font());
  else
    fontSettings.setValue("InputLine", "");

  _fontsChanged = false;

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
    _fontsChanged = true;
    setFont(label, font);
  }
}

void AppearanceSettingsPage::chooseStyleSheet() {
  QString name = QFileDialog::getOpenFileName(this, tr("Please choose a stylesheet file"), QString(), "*.qss");
  if(!name.isEmpty())
    ui.customStyleSheetPath->setText(name);
}

void AppearanceSettingsPage::widgetHasChanged() {
  setChangedState(testHasChanged());
}

bool AppearanceSettingsPage::testHasChanged() {
  if(_fontsChanged) return true; // comparisons are nasty for now

  if(ui.styleComboBox->currentIndex() != ui.styleComboBox->property("storedValue").toInt()) return true;

  if(selectedLocale() != QLocale()) return true; // QLocale() returns the default locale (manipulated via loadTranslation())

  return false;
}

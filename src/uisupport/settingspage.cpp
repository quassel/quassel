/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "settingspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QVariant>

#include "uisettings.h"

SettingsPage::SettingsPage(const QString &category, const QString &title, QWidget *parent)
    : QWidget(parent),
    _category(category),
    _title(title),
    _changed(false),
    _autoWidgetsChanged(false)
{
}


void SettingsPage::setChangedState(bool hasChanged_)
{
    if (hasChanged_ != _changed) {
        bool old = hasChanged();
        _changed = hasChanged_;
        if (hasChanged() != old)
            emit changed(hasChanged());
    }
}


void SettingsPage::load(QCheckBox *box, bool checked)
{
    box->setProperty("StoredValue", checked);
    box->setChecked(checked);
}


bool SettingsPage::hasChanged(QCheckBox *box)
{
    return box->property("StoredValue").toBool() != box->isChecked();
}


void SettingsPage::load(QComboBox *box, int index)
{
    box->setProperty("StoredValue", index);
    box->setCurrentIndex(index);
}


bool SettingsPage::hasChanged(QComboBox *box)
{
    return box->property("StoredValue").toInt() != box->currentIndex();
}


void SettingsPage::load(QSpinBox *box, int value)
{
    box->setProperty("StoredValue", value);
    box->setValue(value);
}


bool SettingsPage::hasChanged(QSpinBox *box)
{
    return box->property("StoredValue").toInt() != box->value();
}


/*** Auto child widget handling ***/

void SettingsPage::initAutoWidgets()
{
    _autoWidgets.clear();

    // find all descendants that should be considered auto widgets
    // we need to climb the QObject tree recursively
    findAutoWidgets(this, &_autoWidgets);

    foreach(QObject *widget, _autoWidgets) {
        if (widget->inherits("ColorButton"))
            connect(widget, SIGNAL(colorChanged(QColor)), SLOT(autoWidgetHasChanged()));
        else if (widget->inherits("QAbstractButton") || widget->inherits("QGroupBox"))
            connect(widget, SIGNAL(toggled(bool)), SLOT(autoWidgetHasChanged()));
        else if (widget->inherits("QLineEdit") || widget->inherits("QTextEdit"))
            connect(widget, SIGNAL(textChanged(const QString &)), SLOT(autoWidgetHasChanged()));
        else if (widget->inherits("QComboBox"))
            connect(widget, SIGNAL(currentIndexChanged(int)), SLOT(autoWidgetHasChanged()));
        else if (widget->inherits("QSpinBox"))
            connect(widget, SIGNAL(valueChanged(int)), SLOT(autoWidgetHasChanged()));
        else if (widget->inherits("FontSelector"))
            connect(widget, SIGNAL(fontChanged(QFont)), SLOT(autoWidgetHasChanged()));
        else
            qWarning() << "SettingsPage::init(): Unknown autoWidget type" << widget->metaObject()->className();
    }
}


void SettingsPage::findAutoWidgets(QObject *parent, QObjectList *autoList) const
{
    foreach(QObject *child, parent->children()) {
        if (child->property("settingsKey").isValid())
            autoList->append(child);
        findAutoWidgets(child, autoList);
    }
}


QByteArray SettingsPage::autoWidgetPropertyName(QObject *widget) const
{
    QByteArray prop;
    if (widget->inherits("ColorButton"))
        prop = "color";
    else if (widget->inherits("QAbstractButton") || widget->inherits("QGroupBox"))
        prop = "checked";
    else if (widget->inherits("QLineEdit") || widget->inherits("QTextEdit"))
        prop = "text";
    else if (widget->inherits("QComboBox"))
        prop = "currentIndex";
    else if (widget->inherits("QSpinBox"))
        prop = "value";
    else if (widget->inherits("FontSelector"))
        prop = "selectedFont";
    else
        qWarning() << "SettingsPage::autoWidgetPropertyName(): Unhandled widget type for" << widget;

    return prop;
}


QString SettingsPage::autoWidgetSettingsKey(QObject *widget) const
{
    QString key = widget->property("settingsKey").toString();
    if (key.isEmpty())
        return QString("");
    if (key.startsWith('/'))
        key.remove(0, 1);
    else
        key.prepend(settingsKey() + '/');
    return key;
}


void SettingsPage::autoWidgetHasChanged()
{
    bool changed_ = false;
    foreach(QObject *widget, _autoWidgets) {
        QVariant curValue = widget->property(autoWidgetPropertyName(widget));
        if (!curValue.isValid())
            qWarning() << "SettingsPage::autoWidgetHasChanged(): Unknown property";

        if (curValue != widget->property("storedValue")) {
            changed_ = true;
            break;
        }
    }

    if (changed_ != _autoWidgetsChanged) {
        bool old = hasChanged();
        _autoWidgetsChanged = changed_;
        if (hasChanged() != old)
            emit changed(hasChanged());
    }
}


void SettingsPage::load()
{
    UiSettings s("");
    foreach(QObject *widget, _autoWidgets) {
        QString key = autoWidgetSettingsKey(widget);
        QVariant val;
        if (key.isEmpty())
            val = loadAutoWidgetValue(widget->objectName());
        else
            val = s.value(key, QVariant());
        if (!val.isValid())
            val = widget->property("defaultValue");
        widget->setProperty(autoWidgetPropertyName(widget), val);
        widget->setProperty("storedValue", val);
    }
    bool old = hasChanged();
    _autoWidgetsChanged = _changed = false;
    if (hasChanged() != old)
        emit changed(hasChanged());
}


void SettingsPage::save()
{
    UiSettings s("");
    foreach(QObject *widget, _autoWidgets) {
        QString key = autoWidgetSettingsKey(widget);
        QVariant val = widget->property(autoWidgetPropertyName(widget));
        widget->setProperty("storedValue", val);
        if (key.isEmpty())
            saveAutoWidgetValue(widget->objectName(), val);
        else
            s.setValue(key, val);
    }
    bool old = hasChanged();
    _autoWidgetsChanged = _changed = false;
    if (hasChanged() != old)
        emit changed(hasChanged());
}


void SettingsPage::defaults()
{
    foreach(QObject *widget, _autoWidgets) {
        QVariant val = widget->property("defaultValue");
        widget->setProperty(autoWidgetPropertyName(widget), val);
    }
    autoWidgetHasChanged();
}


QVariant SettingsPage::loadAutoWidgetValue(const QString &widgetName)
{
    qWarning() << "Could not load value for SettingsPage widget" << widgetName;
    return QVariant();
}


void SettingsPage::saveAutoWidgetValue(const QString &widgetName, const QVariant &)
{
    qWarning() << "Could not save value for SettingsPage widget" << widgetName;
}

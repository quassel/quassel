/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef _SETTINGSPAGE_H_
#define _SETTINGSPAGE_H_

#include <QWidget>

class QCheckBox;
class QComboBox;
class QSpinBox;

//! A SettingsPage is a page in the settings dialog.
/** The SettingsDlg provides suitable standard buttons, such as Ok, Apply, Cancel, Restore Defaults and Reset.
 *  Some pages might also be used in standalone dialogs or other containers. A SettingsPage provides suitable
 *  slots and signals to allow interaction with the container.
 *
 *  A derived class needs to keep track of its changed state. Whenever a child widget changes, it needs to be
 *  compared to its value in permanent storage, and the changed state updated accordingly by calling setChangedState().
 *  For most standard widgets, SettingsPage can do this automatically if desired. Such a child widget needs to have
 *  a dynamic property \c settingsKey that maps to the key in the client configuration file. This key is appended
 *  to settingsKey(), which must be set to a non-null value in a derived class. If the widget's key starts with '/',
 *  its key is treated as a global path starting from the root, rather than from settingsKey().
 *  A second dynamic property \c defaultValue can be defined in child widgets as well.
 *
 *  For widgets requiring special ways for storing and saving, define the property settingsKey and leave it empty. In this
 *  case, the methods saveAutoWidgetValue() and loadAutoWidgetValue() will be called with the widget's objectName as parameter.
 *
 *  SettingsPage manages loading, saving, setting to default and setting the changed state for all automatic child widgets.
 *  Derived classes must be sure to call initAutoWidgets() *after* setupUi(); they also need to call the baseclass implementations
 *  of load(), save() and defaults() (preferably at the end of the derived function, since they call setChangedState(false)).
 *
 *  The following widgets can be handled for now:
 *    - QGroupBox (isChecked())
 *    - QAbstractButton (isChecked(), e.g. for QCheckBox and QRadioButton)
 *    - QLineEdit, QTextEdit (text())
 *    - QComboBox (currentIndex())
 *    - QSpinBox (value())
 */
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    SettingsPage(const QString &category, const QString &name, QWidget *parent = 0);
    virtual ~SettingsPage() {};

    //! The category of this settings page.
    inline virtual QString category() const { return _category; }

    //! The title of this settings page.
    inline virtual QString title() const { return _title; }

    //! Whether the settingspage needs a core connection to be selectable
    /** This is a hint for the settingspage dialog. Do not rely on the settingspage not being
     *  visible if disconnected, and care about disabling it yourself.
     */
    inline virtual bool needsCoreConnection() const { return false; }

    /**
     * Whether the settingspage should be selectable or not, in a given situation
     * Used for pages that should only be visible if certain features are available (or not).
     * @return
     */
    inline virtual bool isSelectable() const { return true; }

    //! The key this settings page stores its values under
    /** This needs to be overriden to enable automatic loading/saving/hasChanged checking of widgets.
     *  The child widgets' values will be stored in client settings under this key. Every widget that
     *  should be automatically handled needs to have a \c settingsKey property set, and should also provide
     *  a \c defaultValue property.
     *  You can return an empty string (as opposed to a null string) to use the config root as a base, and
     *  you can override this key for individual widgets by prefixing their SettingsKey with /.
     */
    inline virtual QString settingsKey() const { return QString(); }

    //! Derived classes need to define this and return true if they have default settings.
    /** If this method returns true, the "Restore Defaults" button in the SettingsDlg is
     *  enabled. You also need to provide an implementation of defaults() then.
     *
     * The default implementation returns false.
       */
    inline virtual bool hasDefaults() const { return false; }

    //! Check if there are changes in the page, compared to the state saved in permanent storage.
    inline bool hasChanged() const { return _changed || _autoWidgetsChanged; }

    //! Called immediately before save() is called.
    /** Derived classes should return false if saving is not possible (e.g. the current settings are invalid).
     *  \return false, if the SettingsPage cannot be saved in its current state.
     */
    inline virtual bool aboutToSave() { return true; }

    //! sets checked state depending on \checked and stores the value for later comparision
    static void load(QCheckBox *box, bool checked);
    static bool hasChanged(QCheckBox *box);
    static void load(QComboBox *box, int index);
    static bool hasChanged(QComboBox *box);
    static void load(QSpinBox *box, int value);
    static bool hasChanged(QSpinBox *box);

public slots:
    //! Save settings to permanent storage.
    /** This baseclass implementation saves the autoWidgets, so be sure to call it if you use
     *  this feature in your settingsPage!
     */
    virtual void save();

    //! Load settings from permanent storage, overriding any changes the user might have made in the dialog.
    /** This baseclass implementation loads the autoWidgets, so be sure to call it if you use
     *  this feature in your settingsPage!
     */
    virtual void load();

    //! Restore defaults, overriding any changes the user might have made in the dialog.
    /** This baseclass implementation loads the defaults of the autoWidgets (if available), so be sure
     *  to call it if you use this feature in your settingsPage!
     */
    virtual void defaults();

protected slots:
    //! Calling this slot is equivalent to calling setChangedState(true).
    inline void changed() { setChangedState(true); }

    //! This should be called whenever the widget state changes from unchanged to change or the other way round.
    void setChangedState(bool hasChanged = true);

protected:
    void initAutoWidgets();
    virtual QVariant loadAutoWidgetValue(const QString &widgetName);
    virtual void saveAutoWidgetValue(const QString &widgetName, const QVariant &value);

signals:
    //! Emitted whenever the widget state changes.
    void changed(bool hasChanged);

private slots:
    // for auto stuff
    void autoWidgetHasChanged();

private:
    void findAutoWidgets(QObject *parent, QObjectList *widgetList) const;
    QByteArray autoWidgetPropertyName(QObject *widget) const;
    QString autoWidgetSettingsKey(QObject *widget) const;

    QString _category, _title;
    bool _changed, _autoWidgetsChanged;
    QObjectList _autoWidgets;
};


#endif

/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef _SETTINGSPAGE_H_
#define _SETTINGSPAGE_H_

#include <QWidget>

class QCheckBox;

//! A SettingsPage is a page in the settings dialog.
/** The SettingsDlg provides suitable standard buttons, such as Ok, Apply, Cancel, Restore Defaults and Reset.
 *  Some pages might also be used in standalone dialogs or other containers. A SettingsPage provides suitable
 *  slots and signals to allow interaction with the container.
 */
class SettingsPage : public QWidget {
  Q_OBJECT

public:
  SettingsPage(const QString &category, const QString &name, QWidget *parent = 0);
  virtual ~SettingsPage() {};
  
  //! The category of this settings page.
  inline virtual QString category() const { return _category; }
  
  //! The title of this settings page.
  inline virtual QString title() const { return _title; }
  
  //! Derived classes need to define this and return true if they have default settings.
  /** If this method returns true, the "Restore Defaults" button in the SettingsDlg is
   *  enabled. You also need to provide an implementation of defaults() then.
   *
   * The default implementation returns false.
     */
  inline virtual bool hasDefaults() const { return false; }
  
  //! Check if there are changes in the page, compared to the state saved in permanent storage.
  inline bool hasChanged() const { return _changed; }
  
  //! Called immediately before save() is called.
  /** Derived classes should return false if saving is not possible (e.g. the current settings are invalid).
   *  \return false, if the SettingsPage cannot be saved in its current state.
   */
  inline virtual bool aboutToSave() { return true; }

  //! sets checked state depending on \checked and stores the value for later comparision
  static void load(QCheckBox *box, bool checked);
  static bool hasChanged(QCheckBox *box);

public slots:
  //! Save settings to permanent storage.
  virtual void save() = 0;
  
  //! Load settings from permanent storage, overriding any changes the user might have made in the dialog.
  virtual void load() = 0;

  //! Restore defaults, overriding any changes the user might have made in the dialog.
  /** The default implementation does nothing.
   */
  inline virtual void defaults() {}
			 
protected slots:
  //! Calling this slot is equivalent to calling setChangedState(true).
  inline void changed() { setChangedState(true); }
  
  //! This should be called whenever the widget state changes from unchanged to change or the other way round.
  void setChangedState(bool hasChanged = true);
  
signals:
  //! Emitted whenever the widget state changes.
  void changed(bool hasChanged);
  
private:
  QString _category, _title;
  bool _changed;
};



#endif

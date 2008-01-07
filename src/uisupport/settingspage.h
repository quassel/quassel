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

//! A SettingsPage is a page in the settings dialog.
class SettingsPage : public QWidget {
  Q_OBJECT

  public:
    SettingsPage(const QString &category, const QString &name, QWidget *parent = 0);
    virtual ~SettingsPage() {};
    virtual QString category() const;
    virtual QString title() const;

    bool hasChanged() const;

    //! Called immediately before save() is called.
    /** Derived classes should return false if saving is not possible (e.g. the current settings are invalid).
     *  \return false, if the SettingsPage cannot be saved in its current state.
     */
    virtual bool aboutToSave();

  public slots:
    virtual void save() = 0;
    virtual void load() = 0;
    virtual void defaults() = 0;

  protected slots:
    //! Calling this slot is equivalent to calling changeState(true).
    void changed();

  protected:
    //! This should be called whenever the widget state changes from unchanged to change or the other way round.
    void changeState(bool hasChanged = true);

  signals:
    //! Emitted whenever the widget state changes.
    void changed(bool hasChanged);

  private:
    QString _category, _title;
    bool _changed;
};

#endif

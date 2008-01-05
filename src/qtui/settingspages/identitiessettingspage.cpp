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

#include "identitiessettingspage.h"

#include "client.h"

IdentitiesSettingsPage::IdentitiesSettingsPage(QWidget *parent)
  : SettingsPage(tr("General"), tr("Identities"), parent) {

  ui.setupUi(this);
  setEnabled(false);  // need a core connection!
  connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));

}

void IdentitiesSettingsPage::coreConnectionStateChanged(bool state) {
  //this->setEnabled(state);
  if(state) {
    load();
  }
}

bool IdentitiesSettingsPage::hasChanged() const {
  return true;
}

void IdentitiesSettingsPage::save() {

}

void IdentitiesSettingsPage::load() {

}

void IdentitiesSettingsPage::defaults() {

}

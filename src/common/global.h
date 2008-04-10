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

// This file needs probably to go away at some point. Not much left anymore.

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <QString>

// Enable some shortcuts and stuff
//#define DEVELMODE

/* Some global stuff */

namespace Global {

  extern QString quasselVersion;
  extern QString quasselDate;
  extern uint quasselBuild;

  //! Minimum client build number the core needs
  extern uint clientBuildNeeded;
  extern QString clientVersionNeeded;

  //! Minimum core build number the client needs
  extern uint coreBuildNeeded;
  extern QString coreVersionNeeded;

  // We need different config (QSettings) files for client and gui, since the core cannot work with GUI types
  // Set these here. They're used in ClientSettings and CoreSettings.
  const QString coreApplicationName = "Quassel Core";
  const QString clientApplicationName = "Quassel Client";

  enum RunMode { Monolithic, ClientOnly, CoreOnly };
  extern RunMode runMode;
  extern unsigned int defaultPort;

  extern bool DEBUG;

  void registerMetaTypes();
};

#endif

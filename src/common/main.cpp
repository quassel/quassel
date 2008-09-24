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

#include <cstdlib>

#ifdef BUILD_CORE
#  include "../core/coreapplication.h"
#elif defined BUILD_QTUI
#  include "qtuiapplication.h"
#elif defined BUILD_MONO
#  include "monoapplication.h"

#else
#error "Something is wrong - you need to #define a build mode!"
#endif

#include "quassel.h"

int main(int argc, char **argv) {

#  ifdef BUILD_CORE
    CoreApplication app(argc, argv);
#  elif BUILD_QTUI
    QtUiApplication app(argc, argv);
#  elif BUILD_MONO
    MonolithicApplication app(argc, argv);
#  endif

  if(!app.init()) return EXIT_FAILURE;
  return app.exec();
}

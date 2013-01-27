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

#ifndef ICON_H_
#define ICON_H_

#ifndef HAVE_KDE

#include <QIcon>

/// A very thin wrapper around QIcon
/** This wrapper class allows us to load an icon by its theme name rather than its full file name.
 *  The overloaded ctor uses IconLoader to locate an icon with this basename in the current theme
 *  or in Qt Resources.
 */
class Icon : public QIcon
{
public:
    Icon();
    explicit Icon(const QString &iconName);
    explicit Icon(const QIcon &copy);

    Icon &operator=(const Icon &other);
};


#else /* HAVE_KDE */
#include <KIcon>
class Icon : public KIcon
{
public:
    inline Icon() : KIcon() {};
    inline explicit Icon(const QString &iconName) : KIcon(iconName) {};
    inline explicit Icon(const QIcon &copy) : KIcon(copy) {};
};


#endif /* HAVE_KDE */

#endif

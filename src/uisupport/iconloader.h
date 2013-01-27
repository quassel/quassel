/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   Based in part on KDE's kiconloader.h                                  *
 *   This declares a subset of that API.                                   *
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

#ifndef ICONLOADER_H_
#define ICONLOADER_H_

#ifndef HAVE_KDE

#include <QPixmap>

/// Provides basic facilities to load icons from standard locations or resources
/** This implements a (very) basic subset of KIconLoader's API, such that we can use those classes
 *  interchangeably in Quassel.
 *
 *  We currently (unless somebody does a more fancy implementation ;-)) assume the Oxygen icon theme
 *  to be used. In particular, this means that we do assume its directory layout and existing icons to
 *  be present. Though it should be easy to switch to a different theme name, we don't currently support
 *  any fallback mechanism if this other theme misses an icon for a given size. Also, we only support PNG.
 *
 *  Since we do support integrating the required part of Oxygen into the binary via Qt Resources, this
 *  should work for everyone for now.
 *
 *  - $XDG_DATA_DIRS/icons/$theme (in order)
 *  - :/icons/$theme (fallback in case we use Qt Resources)
 *  - $XDG_DATA_DIRS/apps/quassel/icons/hicolor (our own unthemed icons)
 *  - :/icons/hicolor
 *  - $XDG_DATA_DIRS/apps/quassel/pics
 *  - :/pics
 *
 *  We don't search for size/context dirs in /pics, i.e. for a given $name, we expect pics/$name.png.
 */
class IconLoader : public QObject
{
    Q_OBJECT

public:
    enum Group {
        NoGroup = -1, ///< No group
        Desktop = 0, ///< Desktop icons
        Toolbar,    ///< Toolbar icons
        MainToolbar, ///< Main toolbar icons
        Small,      ///< Small icons, e.g. for buttons
        Panel,      ///< Panel icons
        Dialog,     ///< Icons for use in dialog title etc.
        LastGroup
    };

    /// Standard icon sizes
    enum StdSizes {
        SizeSmall = 16,   ///< Small icons for menu entries
        SizeSmallMedium = 22, ///< Slightly larger small icons for toolbars, panels, etc
        SizeMedium = 32,  ///< Medium-sized icons for the desktop
        SizeLarge = 48,   ///< Large icons for the panel
        SizeHuge = 64,    ///< Huge icons for iconviews
        SizeEnormous = 128 ///< Enormous icons for iconviews
    };

    explicit IconLoader(QObject *parent = 0);
    ~IconLoader();

    static IconLoader *global();

    /// Load a pixmap for the given name and group
    QPixmap loadIcon(const QString &name, IconLoader::Group group, int size = 0);

    inline QString theme() const;
    void setTheme(const QString &name);

private:
    QString findIconPath(const QString &name, int size);

    static IconLoader _iconLoader;
    QString _theme;
    QStringList _themedIconDirNames;
    QStringList _plainIconDirNames;
    static int _groupSize[];
};


// convenience
QPixmap DesktopIcon(const QString &name, int size = 0);
QPixmap BarIcon(const QString &name, int size = 0);
QPixmap MainBarIcon(const QString &name, int size = 0);
QPixmap SmallIcon(const QString &name, int size = 0);
//QPixmap SmallMediumIcon(const QString &name, int size = 0);  // not part of KIconLoader

QString IconLoader::theme() const { return _theme; }

#else /* HAVE_KDE */

#include <KIconLoader>
class IconLoader : public KIconLoader
{
    Q_OBJECT
};


#endif /* HAVE_KDE */

#endif

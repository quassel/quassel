# Icon theme support in Quassel IRC

## Introduction

Quassel IRC requires many icons, which generally fall in three categories:

 - Standard icons as defined by the [Icon Naming
Specification][icon-naming-spec]. These icons should be available in most modern
icon themes.
 - Extended icons that are not part of the standard, but provided by the
[Breeze][breeze-icons] icon theme that is the default in the KDE/Plasma desktop.
As a KDE-affine application, Quassel IRC makes use of this extended set of
icons.
 - Quassel-specific icons that are not part of any official icon theme.

We have put great effort into supporting different icon themes in the Quassel
client -- with the caveat that things may not look perfectly consistent, because
we need to fill the gaps in the system icon theme with icons that are shipped
with Quassel.

## Icon theme support

### Supported icon themes

Quassel directly supports the [Breeze][breeze-icons] and [Breeze Dark][breeze-icons]
icon themes that are the default in KDE/Plasma, and
optionally the [Oxygen][oxygen-icons] icon theme that was the default in KDE4.
This means that Quassel-specific icons are available to match either of the
three themes, and that Quassel should integrate seamlessly into your desktop's
look-and-feel if your system icon theme is one of the three. On platforms that
don't support system icon themes at all (e.g. Windows and Mac OS), users may
select the theme to use in the Appearance settings.

The default icon theme for Quassel is Breeze, and thus for example the
application icon will always come from this theme.

### Handling icon themes that are not directly supported by Quassel

As stated before, most of the unsupported icon themes will be missing icons that
Quassel requires, due to them not being specified in the [Icon Naming
Specification][icon-naming-spec]. To cope with this, Quassel ships the required
subset of each of the supported themes, and injects them as a fallback into the
icon loading mechanism.

In the Appearance settings, users can choose which fallback theme to use, and if
Quassel should completely override the system theme (to get a consistent
look-and-feel inside the client), or just fill the gaps (to get a more
consistent look with the desktop environment).

### Overriding icons

Users may override any icon by providing a theme directory (matching their
system theme's name and directory layout) in one of the default theme search
paths (on Linux, the appropriate place would be `$HOME/.local/share/icons`), and
putting the replacement icons there.

## Technical information

### Build options

By default, Quassel IRC will install the bundled Breeze and Breeze Dark icon
themes (which are not the full upstream themes, but only the subset actually
required by Quassel). This adds around 1.7 MiB to the installed size. However,
if it is ensured that the Breeze theme is available on the system (e.g. through
package dependencies), installing the bundled icons can be disabled using the
CMake option `-DWITH_BUNDLED_ICONS=OFF`.

Note that the themes are installed into Quassel's own data directory, so they
will not clash with a system installation.

The Oxygen icon theme is deprecated, and thus support for it is not enabled by
default. By setting the CMake option `-DWITH_OXYGEN_ICONS=ON`, Oxygen icons will
be installed. This adds around 1.8 MiB for the Quassel-specific icons, and
another 2.6 MiB if the bundled icon theme is installed as well.

### Icon directories

Quassel-specific icons are located in `icons/`. Note that the Breeze variants
are generated from the  [quassel-icons repository][quassel-icons] and should not
be updated manually.

Bundled icon theme subsets are provided in `3rdparty/icons/`. These themes can
be updated from the upstream theme by running the `icons/import/import_theme.pl`
script. See the script header for usage instructions.

## Known issues

 - Qt versions older than Qt 5.5 don't support split icon theme installations.
Thus, the fallback mechanism does not work correctly, and icons will most likely
be missing. Third-party theme support does not work correctly, either.
 - Qt currently has a [bug][QTBUG-68603] where it prefers the standard `hicolor`
theme over inherited themes. Due to the way Quassel's icon loading mechanism
deals with unsupported themes, this means that an icon available in hicolor will
override the one provided in the selected fallback theme. This normally affects
only the Quassel-specific icons (most visisbly the tray icon), which will then
always come from Breeze.

[icon-naming-spec]: https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
[breeze-icons]: https://github.com/KDE/breeze-icons
[oxygen-icons]: https://github.com/KDE/oxygen-icons
[quassel-icons]: https://github.com/justjanne/quassel-icons
[QTBUG-68603]: https://bugreports.qt.io/browse/QTBUG-68603

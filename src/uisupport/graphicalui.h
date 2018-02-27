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

#ifndef GRAPHICALUI_H_
#define GRAPHICALUI_H_

#include "abstractui.h"
#include "types.h"

class ActionCollection;
class ContextMenuActionProvider;
class ToolBarActionProvider;
class UiStyle;

#ifdef Q_OS_WIN
#  include <windows.h>
#endif
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

class GraphicalUi : public AbstractUi
{
    Q_OBJECT

public:
    GraphicalUi(QObject *parent = 0);
    virtual void init();

    //! Access global ActionCollections.
    /** These ActionCollections are associated with the main window, i.e. they contain global
    *  actions (and thus, shortcuts). Widgets providing application-wide shortcuts should
    *  create appropriate Action objects using GraphicalUi::actionCollection(cat)->add\<Action\>().
    *  @param category The category (default: "General")
    */
    static ActionCollection *actionCollection(const QString &category = "General", const QString &translatedCategory = QString());
    static ActionCollection *quickAccessorActionCollection(const QString&);
    static QHash<QString, ActionCollection *> actionCollections();
    static QHash<QString, ActionCollection *> quickAccessorActionCollections();
    static QHash<QString, ActionCollection *> allActionCollections();

    //! Load custom shortcuts from ShortcutSettings
    /** @note This method assumes that all configurable actions are defined when being called
     */
    static void loadShortcuts();

    //! Save custom shortcuts to ShortcutSettings
    static void saveShortcuts();

    inline static ContextMenuActionProvider *contextMenuActionProvider();
    inline static ToolBarActionProvider *toolBarActionProvider();
    inline static UiStyle *uiStyle();
    inline static QWidget *mainWidget();

    //! Force the main widget to the front and focus it (may not work in all window systems)
    static void activateMainWidget();

    //! Hide main widget (storing the current desktop if possible)
    static void hideMainWidget();

    //! Toggle main widget
    static void toggleMainWidget();

    //! Check if the main widget if (fully, in KDE) visible
    static bool isMainWidgetVisible();

protected:
    //! This is the widget we associate global actions with, typically the main window
    void setMainWidget(QWidget *);

    //! Check if the mainWidget is visible and optionally toggle its visibility
    /** With KDE integration, we check if the mainWidget is (partially) obscured in order to determine if
     *  it should be activated or hidden. Without KDE, we need to resort to checking the current state
     *  as Qt knows it, ignoring windows covering it.
     *  @param  performToggle If true, toggle the window's state in addition to checking visibility
     *  @return True, if the window is currently *not* visible (needs activation)
     */
    bool checkMainWidgetVisibility(bool performToggle);

    //! Minimize to or restore main widget
    virtual void minimizeRestore(bool show);

    //! Whether it is allowed to hide the mainWidget
    /** The default implementation returns false, meaning that we won't hide the mainWidget even
     *  if requested. This is to prevent hiding in case we don't have a tray icon to restore from.
     */
    virtual inline bool isHidingMainWidgetAllowed() const;

    void setContextMenuActionProvider(ContextMenuActionProvider *);
    void setToolBarActionProvider(ToolBarActionProvider *);
    void setUiStyle(UiStyle *);

    virtual bool eventFilter(QObject *obj, QEvent *event);

protected slots:
    virtual void disconnectedFromCore();

private:
    static inline GraphicalUi *instance();

    static GraphicalUi *_instance;
    static QWidget *_mainWidget;
    static QHash<QString, ActionCollection *> _actionCollections;
    static QHash<QString, ActionCollection *> _quickAccessorActionCollections;
    static ContextMenuActionProvider *_contextMenuActionProvider;
    static ToolBarActionProvider *_toolBarActionProvider;
    static UiStyle *_uiStyle;
    static bool _onAllDesktops;

#ifdef Q_OS_WIN
    DWORD _dwTickCount;
#endif
#ifdef Q_OS_MAC
    ProcessSerialNumber _procNum;
#endif
};


// inlines

GraphicalUi *GraphicalUi::instance() { return _instance; }
ContextMenuActionProvider *GraphicalUi::contextMenuActionProvider() { return _contextMenuActionProvider; }
ToolBarActionProvider *GraphicalUi::toolBarActionProvider() { return _toolBarActionProvider; }
UiStyle *GraphicalUi::uiStyle() { return _uiStyle; }
QWidget *GraphicalUi::mainWidget() { return _mainWidget; }
bool GraphicalUi::isHidingMainWidgetAllowed() const { return false; }

#endif

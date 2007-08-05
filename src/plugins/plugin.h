/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

// TODO disable GUI-related stuff for core-only compile

#include <QtCore>
#include <QtGui>

/** \file plugin.h
 * Header that needs to be included by all plugins and defines the interfaces a plugin
 * can implement.
 */

/// The base class for the generic plugin interfaces.
class PluginInterface {


};


/// All GUI plugins need to implement this interface.
class GuiPluginInterface : public PluginInterface {


};

Q_DECLARE_INTERFACE(GuiPluginInterface,
                    "eu.quassel.plugins.GuiPluginInterface/1.0");

/// All core plugins need to implement this interface.
class CorePluginInterface: public PluginInterface {



};

Q_DECLARE_INTERFACE(CorePluginInterface,
                    "eu.quassel.plugins.CorePluginInterface/1.0");

/** Plugins implementing this interface can provide a settings widget that will be shown in
 * the application's settings dialog.
 * This is also used by built-in settings dialogs.
 */
class SettingsInterface {
  public:
    virtual ~SettingsInterface() {};
    virtual QString category() = 0;
    virtual QString title() = 0; 
    virtual QWidget *settingsWidget() = 0;
    virtual void applyChanges() = 0;

};

Q_DECLARE_INTERFACE(SettingsInterface,
                    "eu.quassel.plugins.SettingsInterface/1.0");

/** Plugins implementing this interface will be provided with the raw text the users enters.
 * The output they generate is in turn treated like generic user input. Note that the order in
 * which plugins are called is not defined.
 */
class UserInputFilterInterface {



};

Q_DECLARE_INTERFACE(UserInputFilterInterface,
                    "eu.quassel.plugins.UserInputFilterInterface/1.0");

/** Plugins implementing this interface receive and can process all messages coming from the core.
 */
class CoreMessageFilterInterface {



};

Q_DECLARE_INTERFACE(CoreMessageFilterInterface,
                    "eu.quassel.plugins.CoreMessageFilterInterface/1.0");

/** Plugins implementing this interface receive core messages for a single buffer only.
 * Any ChatWidgetPlugin has to provide a widget it outputs these messages to, to be used
 * in a ChannelWidget.
 */
class ChatWidgetInterface {



};

Q_DECLARE_INTERFACE(ChatWidgetInterface,
                    "eu.quassel.plugins.ChatWidgetInterface/1.0");

#endif

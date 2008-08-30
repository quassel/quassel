/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef ACTION_H_
#define ACTION_H_

#include <QWidgetAction>

class Action : public QWidgetAction {
  Q_OBJECT

  Q_PROPERTY(QShortcut shortcut READ shortcut WRITE setShortcut)
  Q_PROPERTY(bool shortcutConfigurable READ isShortcutConfigurable WRITE setShortcutConfigurable)
  Q_FLAGS(ShortcutType)

  public:
    enum ShortcutType {
      ActiveShortcut = 0x01,
      DefaultShortcut = 0x02
    };
    Q_DECLARE_FLAGS(ShortcutTypes, ShortcutType)

    explicit Action(QObject *parent);
    Action(const QString &text, QObject *parent);
    Action(const QIcon &icon, const QString &text, QObject *parent);

    QShortcut shortcut(ShortcutTypes types = ActiveShortcut) const;
    void setShortcut(const QShortcut &shortcut, ShortcutTypes type = ActiveShortcut);
    void setShortcut(const QKeySequence &shortcut, ShortcutTypes type = ActiveShortcut);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(ShortcutTypes)

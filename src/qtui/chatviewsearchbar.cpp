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

#include "chatviewsearchbar.h"

#include "action.h"
#include "actioncollection.h"
#include "iconloader.h"
#include "qtui.h"

ChatViewSearchBar::ChatViewSearchBar(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    ui.hideButton->setIcon(BarIcon("dialog-close"));
    ui.searchUpButton->setIcon(SmallIcon("go-up"));
    ui.searchDownButton->setIcon(SmallIcon("go-down"));
    _searchDelayTimer.setSingleShot(true);

    layout()->setContentsMargins(0, 0, 0, 0);

    hide();

    ActionCollection *coll = QtUi::actionCollection("General");

    QAction *toggleSearchBar = coll->action("ToggleSearchBar");
    connect(toggleSearchBar, SIGNAL(toggled(bool)), SLOT(setVisible(bool)));

    Action *hideSearchBar = coll->add<Action>("HideSearchBar", toggleSearchBar, SLOT(setChecked(bool)));
    hideSearchBar->setShortcutConfigurable(false);
    hideSearchBar->setShortcut(Qt::Key_Escape);

    connect(ui.hideButton, SIGNAL(clicked()), toggleSearchBar, SLOT(toggle()));
    connect(ui.searchEditLine, SIGNAL(textChanged(const QString &)), this, SLOT(delaySearch()));
    connect(&_searchDelayTimer, SIGNAL(timeout()), this, SLOT(search()));
}


void ChatViewSearchBar::setVisible(bool visible)
{
    // clearing the search field also removes the highlight items from the scene
    // this should be done before the SearchBar is hidden, as the hiding triggers
    // a resize event which can lead to strange side effects.
    ui.searchEditLine->clear();
    QWidget::setVisible(visible);
    if (visible)
        ui.searchEditLine->setFocus();
    else
        emit hidden();
}


void ChatViewSearchBar::delaySearch()
{
    _searchDelayTimer.start(300);
}


void ChatViewSearchBar::search()
{
    emit searchChanged(ui.searchEditLine->text());
}

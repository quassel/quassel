/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
#include "icon.h"
#include "qtui.h"

ChatViewSearchBar::ChatViewSearchBar(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    ui.hideButton->setIcon(icon::get("dialog-close"));
    ui.searchUpButton->setIcon(icon::get("go-up"));
    ui.searchDownButton->setIcon(icon::get("go-down"));
    _searchDelayTimer.setSingleShot(true);

    layout()->setContentsMargins(0, 0, 0, 0);

    hide();

    ActionCollection *coll = QtUi::actionCollection("General");

    QAction *toggleSearchBar = coll->action("ToggleSearchBar");
    connect(toggleSearchBar, &QAction::toggled, this, &QWidget::setVisible);

    auto *hideSearchBar = coll->add<Action>("HideSearchBar", toggleSearchBar, SLOT(setChecked(bool)));
    hideSearchBar->setShortcutConfigurable(false);
    hideSearchBar->setShortcut(Qt::Key_Escape);

    connect(ui.hideButton, &QAbstractButton::clicked, toggleSearchBar, &QAction::toggle);
    connect(ui.searchEditLine, &QLineEdit::textChanged, this, &ChatViewSearchBar::delaySearch);
    connect(&_searchDelayTimer, &QTimer::timeout, this, &ChatViewSearchBar::search);
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

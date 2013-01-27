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

#ifndef CHATVIEWSEARCHBAR_H
#define CHATVIEWSEARCHBAR_H

#include "ui_chatviewsearchbar.h"

#include <QWidget>
#include <QTimer>

class QAction;

class ChatViewSearchBar : public QWidget
{
    Q_OBJECT

public:
    ChatViewSearchBar(QWidget *parent = 0);

    inline QLineEdit *searchEditLine() const { return ui.searchEditLine; }
    inline QCheckBox *caseSensitiveBox() const { return ui.caseSensitiveBox; }
    inline QCheckBox *searchSendersBox() const { return ui.searchSendersBox; }
    inline QCheckBox *searchMsgsBox() const { return ui.searchMsgsBox; }
    inline QCheckBox *searchOnlyRegularMsgsBox() const { return ui.searchOnlyRegularMsgsBox; }
    inline QToolButton *searchUpButton() const { return ui.searchUpButton; }
    inline QToolButton *searchDownButton() const { return ui.searchDownButton; }

public slots:
    void setVisible(bool);

signals:
    void searchChanged(const QString &);
    void hidden();

private slots:
    void delaySearch();
    void search();

private:
    Ui::ChatViewSearchBar ui;
    QTimer _searchDelayTimer;
};


#endif //CHATVIEWSEARCHBAR_H

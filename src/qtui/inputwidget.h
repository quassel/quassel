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

#ifndef INPUTWIDGET_H
#define INPUTWIDGET_H

#include "ui_inputwidget.h"

#include "abstractitemview.h"
#include "buffermodel.h"
#include "bufferinfo.h"
#include "identity.h"
#include "network.h"
#include <action.h>

class MultiLineEdit;

class InputWidget : public AbstractItemView
{
    Q_OBJECT

public:
    InputWidget(QWidget *parent = 0);
    virtual ~InputWidget();

    const Network *currentNetwork() const;

    inline MultiLineEdit *inputLine() const { return ui.inputEdit; }

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

protected slots:
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private slots:
    void setCustomFont(const QVariant &font);
    void setUseCustomFont(const QVariant &);
    void setEnableSpellCheck(const QVariant &);
    void setEnableEmacsMode(const QVariant &);
    void setShowNickSelector(const QVariant &);
    void setShowStyleButtons(const QVariant &);
    void setEnablePerChatHistory(const QVariant &);
    void setMaxLines(const QVariant &);
    void setMultiLineEnabled(const QVariant &);
    void setScrollBarsEnabled(const QVariant &);
    void onTextEntered(const QString &text);
    void changeNick(const QString &newNick) const;

    void setNetwork(NetworkId networkId);
    void setIdentity(IdentityId identityId);
    void connectMyIrcUser();
    void updateNickSelector() const;
    void updateEnabledState();

    BufferInfo currentBufferInfo() const;

    void currentCharFormatChanged(const QTextCharFormat &format);
    void on_showStyleButton_toggled(bool checked);
    void on_boldButton_clicked(bool checked);
    void on_italicButton_clicked(bool checked);
    void on_underlineButton_clicked(bool checked);
    void colorChosen(QAction *action);
    void colorHighlightChosen(QAction *action);

private:
    Ui::InputWidget ui;

    NetworkId _networkId;
    IdentityId _identityId;
    QMenu *_colorMenu, *_colorFillMenu;

    void mergeFormatOnSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    QIcon createColorToolButtonIcon(const QIcon &icon, const QColor &color);
    QTextCharFormat getFormatOfWordOrSelection();
    void setFormatOnSelection(const QTextCharFormat &format);

    bool _perChatHistory;
    struct HistoryState {
        QStringList history;
        QHash<int, QString> tempHistory;
        qint32 idx;
        QString inputLine;
        inline HistoryState() : idx(0) {};
    };

    QMap<BufferId, HistoryState> historyMap;
};


class MouseWheelFilter : public QObject
{
    Q_OBJECT

public:
    MouseWheelFilter(QObject *parent = 0);
    virtual bool eventFilter(QObject *obj, QEvent *event);
};


#endif // INPUTWIDGET_H

/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#pragma once

#include <QHash>
#include <QIcon>
#include <QMap>
#include <QTextCharFormat>

#include "abstractitemview.h"
#include "action.h"
#include "bufferinfo.h"
#include "buffermodel.h"
#include "identity.h"
#include "network.h"

#include "ui_inputwidget.h"

class MultiLineEdit;

class InputWidget : public AbstractItemView
{
    Q_OBJECT

public:
    InputWidget(QWidget* parent = nullptr);

    const Network* currentNetwork() const;

    inline MultiLineEdit* inputLine() const { return ui.inputEdit; }

public slots:
    /**
     * Apply the active color to the selected or typed text
     *
     * Active color is chosen using the UI menu.
     */
    void applyFormatActiveColor();

    /**
     * Apply the active fill color to the selected or typed text background
     *
     * Fill color is chosen using the UI menu.
     */
    void applyFormatActiveColorFill();

    /**
     * Toggle the boldness of the selected or typed text
     *
     * Bold becomes normal, and normal becomes bold.
     */
    void toggleFormatBold();

    /**
     * Toggle the italicness of the selected or typed text
     *
     * Italicized becomes normal, and normal becomes italicized.
     */
    void toggleFormatItalic();

    /**
     * Toggle the underlining of the selected or typed text
     *
     * Underlined becomes normal, and normal becomes underlined.
     */
    void toggleFormatUnderline();

    /**
     * Clear the formatting of the selected or typed text
     *
     * Clears the font weight (bold, italic, underline) and foreground/background coloring.
     */
    void clearFormat();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

protected slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) override;

private slots:
    void setCustomFont(const QVariant& font);
    void setUseCustomFont(const QVariant&);
    void setEnableEmacsMode(const QVariant&);
    void setShowNickSelector(const QVariant&);
    void setShowStyleButtons(const QVariant&);
    void setEnablePerChatHistory(const QVariant&);
    void setMaxLines(const QVariant&);
    void setLineWrapEnabled(const QVariant&);
    void setMultiLineEnabled(const QVariant&);
    void setScrollBarsEnabled(const QVariant&);
    void onTextEntered(const QString& text);
    void changeNick(const QString& newNick) const;

    void setNetwork(NetworkId networkId);
    void setIdentity(IdentityId identityId);
    void connectMyIrcUser();
    void updateNickSelector() const;
    void updateEnabledState();

    BufferInfo currentBufferInfo() const;

    /**
     * Set whether or not the style options frame is expanded
     *
     * @param visible If true, expand the style options frame, otherwise collapse it
     */
    void setStyleOptionsExpanded(const bool visible);

    void currentCharFormatChanged(const QTextCharFormat& format);
    void on_showStyleButton_toggled(bool checked);
    void on_boldButton_clicked(bool checked);
    void on_italicButton_clicked(bool checked);
    void on_underlineButton_clicked(bool checked);
    void colorChosen(QAction* action);
    void colorHighlightChosen(QAction* action);

private:
    /**
     * Clear the formatting of the text, globally or selected text only
     *
     * Clears the font weight (bold, italic, underline) and foreground/background coloring.
     *
     * @param global If true, clear all text formatting, otherwise only clear selected text
     */
    void setFormatClear(const bool global = false);

    /**
     * Sets the boldness of the selected or typed text
     *
     * @param bold If true, set text bold, otherwise set text normal
     */
    void setFormatBold(const bool bold);

    /**
     * Sets the italicness of the selected or typed text
     *
     * @param bold If true, set text italic, otherwise set text normal
     */
    void setFormatItalic(const bool italic);

    /**
     * Sets the underline of the selected or typed text
     *
     * @param bold If true, set text underlined, otherwise set text normal
     */
    void setFormatUnderline(const bool underline);

    Ui::InputWidget ui;

    NetworkId _networkId;
    IdentityId _identityId;
    QMenu *_colorMenu, *_colorFillMenu;

    void mergeFormatOnSelection(const QTextCharFormat& format);
    void fontChanged(const QFont& f);
    QIcon createColorToolButtonIcon(const QIcon& icon, const QColor& color);
    QTextCharFormat getFormatOfWordOrSelection();
    void setFormatOnSelection(const QTextCharFormat& format);

    bool _perChatHistory;
    struct HistoryState
    {
        QStringList history;
        QHash<int, QString> tempHistory;
        qint32 idx{0};
        QString inputLine;
    };

    QMap<BufferId, HistoryState> historyMap;
};

class MouseWheelFilter : public QObject
{
    Q_OBJECT

public:
    MouseWheelFilter(QObject* parent = nullptr);
    bool eventFilter(QObject* obj, QEvent* event) override;
};

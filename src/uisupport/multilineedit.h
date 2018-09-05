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

#pragma once

#include "uisupport-export.h"

#include <QKeyEvent>
#include <QHash>

#if defined HAVE_KF5
#  include <KTextWidgets/KTextEdit>
#  define MultiLineEditParent KTextEdit
#else
#  include <QTextEdit>
#  define MultiLineEditParent QTextEdit
#endif

#if defined HAVE_SONNET && !defined HAVE_KDE
#  include <QContextMenuEvent>
#  include <Sonnet/Highlighter>
#  include <Sonnet/SpellCheckDecorator>
#endif

class UISUPPORT_EXPORT MultiLineEdit : public MultiLineEditParent
{
    Q_OBJECT

public:
    enum Mode {
        SingleLine,
        MultiLine
    };

    MultiLineEdit(QWidget *parent = nullptr);
    ~MultiLineEdit();

    void setCustomFont(const QFont &); // should be used instead setFont(), so we can set our size correctly

    // Compatibility methods with the rest of the classes which still expect this to be a QLineEdit
    inline QString text() const { return toPlainText(); }
    inline QString html() const { return toHtml(); }
    inline int cursorPosition() const { return textCursor().position(); }
    inline void insert(const QString &newText) { insertPlainText(newText); }
    inline void backspace() { keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier)); }
    inline bool hasSelectedText() const { return textCursor().hasSelection(); }

    inline bool isSingleLine() const { return _singleLine; }
    inline bool pasteProtectionEnabled() const { return _pasteProtectionEnabled; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    inline QString mircColorFromRGB(QString rgbColor) const { return _mircColorMap.key(rgbColor); }
    inline QString rgbColorFromMirc(QString mircColor) const { return _mircColorMap[mircColor]; }
    inline QMap<QString, QString>  mircColorMap() const { return _mircColorMap; }

    inline QStringList history() const { return _history; }
    inline QHash<int, QString> tempHistory() const { return _tempHistory; }
    inline qint32 idx() const { return _idx; }
    inline bool emacsMode() const { return _emacsMode; }

    void addCompletionSpace();

public slots:
    void setMode(Mode mode);
    void setMinHeight(int numLines);
    void setMaxHeight(int numLines);
    void setEmacsMode(bool enable = true);
    void setScrollBarsEnabled(bool enable = true);
    void setPasteProtectionEnabled(bool enable = true, QWidget *msgBoxParent = nullptr);
    void setLineWrapEnabled(bool enable = false);

    inline void setHistory(QStringList history) { _history = history; }
    inline void setTempHistory(QHash<int, QString> tempHistory) { _tempHistory = tempHistory; }
    inline void setIdx(qint32 idx) { _idx = idx; }

signals:
    void textEntered(const QString &text);
    void noTextEntered();

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

#if defined HAVE_SONNET && !defined HAVE_KDE
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif

private slots:
    void on_returnPressed();
    void on_returnPressed(QString text);
    void on_textChanged();
    void on_documentHeightChanged(qreal height);

    bool addToHistory(const QString &text, bool temporary = false);
    void historyMoveForward();
    void historyMoveBack();

    QString convertRichtextToMircCodes();
    QString convertMircCodesToHtml(const QString &text);
    bool mircCodesChanged(QTextCursor &cursor, QTextCursor &peekcursor);

private:
    void reset();
    void showHistoryEntry();
    void updateScrollBars();
    void updateSizeHint();

private:
    QStringList _history;
    QHash<int, QString> _tempHistory;
    qint32 _idx;
    Mode _mode;
    bool _singleLine;
    int _minHeight;
    int _maxHeight;
    bool _scrollBarsEnabled;
    bool _pasteProtectionEnabled;
    bool _emacsMode;
    int _completionSpace;

    QSize _sizeHint;
    qreal _lastDocumentHeight;

    QMap<QString, QString> _mircColorMap;

#if defined HAVE_SONNET && !defined HAVE_KDE
    // This member function is provided by KTextEdit
    Sonnet::Highlighter *highlighter() const;

private slots:
    void setSpellCheckEnabled(bool enabled);

private:
    Sonnet::SpellCheckDecorator *_spellCheckDecorator{nullptr};
#endif
};

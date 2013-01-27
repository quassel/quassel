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

#ifndef MULTILINEEDIT_H_
#define MULTILINEEDIT_H_

#include <QKeyEvent>
#include <QHash>

#ifdef HAVE_KDE
#  include <KDE/KTextEdit>
#  define MultiLineEditParent KTextEdit
#else
#  include <QTextEdit>
#  define MultiLineEditParent QTextEdit
#endif

class QKeyEvent;
class TabCompleter;

class MultiLineEdit : public MultiLineEditParent
{
    Q_OBJECT

public:
    enum Mode {
        SingleLine,
        MultiLine
    };

    MultiLineEdit(QWidget *parent = 0);
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

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    inline QString mircColorFromRGB(QString rgbColor) const { return _mircColorMap.key(rgbColor); }
    inline QString rgbColorFromMirc(QString mircColor) const { return _mircColorMap[mircColor]; }
    inline QMap<QString, QString>  mircColorMap() const { return _mircColorMap; }

    inline QStringList history() const { return _history; }
    inline QHash<int, QString> tempHistory() const { return _tempHistory; }
    inline qint32 idx() const { return _idx; }
    inline bool emacsMode() const { return _emacsMode; }

public slots:
    void setMode(Mode mode);
    void setMinHeight(int numLines);
    void setMaxHeight(int numLines);
    void setEmacsMode(bool enable = true);
    void setScrollBarsEnabled(bool enable = true);
    void setSpellCheckEnabled(bool enable = true);
    void setPasteProtectionEnabled(bool enable = true, QWidget *msgBoxParent = 0);

    // Note: Enabling wrap will make isSingleLine() not work correctly, so only use this if minHeight() > 1!
    void setWordWrapEnabled(bool enable = true);

    inline void setHistory(QStringList history) { _history = history; }
    inline void setTempHistory(QHash<int, QString> tempHistory) { _tempHistory = tempHistory; }
    inline void setIdx(qint32 idx) { _idx = idx; }

signals:
    void textEntered(const QString &text);
    void noTextEntered();

protected:
    virtual bool event(QEvent *e);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void on_returnPressed();
    void on_returnPressed(const QString &text);
    void on_textChanged();
    void on_documentHeightChanged(qreal height);

    bool addToHistory(const QString &text, bool temporary = false);
    void historyMoveForward();
    void historyMoveBack();

    QString convertRichtextToMircCodes();
    QString convertMircCodesToHtml(const QString &text);
    bool mircCodesChanged(QTextCursor &cursor, QTextCursor &peekcursor);

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

    QSize _sizeHint;
    qreal _lastDocumentHeight;

    QMap<QString, QString> _mircColorMap;

    void reset();
    void showHistoryEntry();
    void updateScrollBars();
    void updateSizeHint();
};


#endif

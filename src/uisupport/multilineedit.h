/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef MULTILINEEDIT_H_
#define MULTILINEEDIT_H_

#include <QKeyEvent>
#include <QHash>
#include <QTextEdit>

#ifdef HAVE_KDE
#  include <KDE/KTextEdit>
#endif

class QKeyEvent;
class TabCompleter;

class MultiLineEdit : public
#ifdef HAVE_KDE
                  KTextEdit
#else
                  QTextEdit
#endif
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
  inline QString text() { return toPlainText(); }
  inline int cursorPosition() { return textCursor().position(); }
  inline void insert(const QString &newText) { insertPlainText(newText); }
  inline void backspace() { keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier)); }
  inline bool hasSelectedText() { return textCursor().hasSelection(); }

  inline bool isSingleLine() const { return _singleLine; }
  inline bool pasteProtectionEnabled() const { return _pasteProtectionEnabled; }

  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

public slots:
  void setMode(Mode mode);
  void setMinHeight(int numLines);
  void setMaxHeight(int numLines);
  void setScrollBarsEnabled(bool enable = true);
  void setSpellCheckEnabled(bool enable = true);
  void setPasteProtectionEnabled(bool enable = true, QWidget *msgBoxParent = 0);

  // Note: Enabling wrap will make isSingleLine() not work correctly, so only use this if minHeight() > 1!
  void setWordWrapEnabled(bool enable = true);

signals:
  void textEntered(const QString &text);
  void noTextEntered();

protected:
  virtual void keyPressEvent(QKeyEvent * event);
  virtual void resizeEvent(QResizeEvent *event);

private slots:
  void on_returnPressed();
  void on_returnPressed(const QString &text);
  void on_textChanged();
  void on_documentHeightChanged(qreal height);

  bool addToHistory(const QString &text, bool temporary = false);
  void historyMoveForward();
  void historyMoveBack();

private:
  QStringList history;
  QHash<int, QString> tempHistory;
  qint32 idx;
  Mode _mode;
  bool _singleLine;
  int _minHeight;
  int _maxHeight;
  bool _scrollBarsEnabled;
  bool _pasteProtectionEnabled;

  QSize _sizeHint;
  qreal _lastDocumentHeight;

  void reset();
  void showHistoryEntry();
  void updateScrollBars();
  void updateSizeHint();
};

#endif

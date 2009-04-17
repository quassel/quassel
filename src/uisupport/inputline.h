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

#ifndef INPUTLINE_H_
#define INPUTLINE_H_

#include <QHash>
#include <QKeyEvent>
#include <QTextEdit>

#ifdef HAVE_KDE
#include <KDE/KTextEdit>
#endif

class TabCompleter;

class InputLine : public
#ifdef HAVE_KDE
                  KTextEdit
#else
                  QTextEdit
#endif
{
  Q_OBJECT

public:
  InputLine(QWidget *parent = 0);
  ~InputLine();

  void setCustomFont(const QFont &); // should be used instead setFont(), so we can set our size correctly

  // Compatibility methods with the rest of the classes which still expect this to be a QLineEdit
  inline QString text() { return toPlainText(); }
  inline int cursorPosition() { return textCursor().position(); }
  inline void insert(const QString &newText) { insertPlainText(newText); }
  inline void backspace() { keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier)); }
  inline bool hasSelectedText() { return textCursor().hasSelection(); }

  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

protected:
  virtual void keyPressEvent(QKeyEvent * event);
  virtual bool eventFilter(QObject *watched, QEvent *event);

private slots:
  void on_returnPressed();
  void on_textChanged(QString newText);

  // Needed to emulate the signal that QLineEdit has
  inline void on_textChanged() { emit textChanged(toPlainText()); };

  bool addToHistory(const QString &text, bool temporary = false);

signals:
  void sendText(QString text);

  // QTextEdit does not provide this signal, so we manually emit it in keyPressEvent()
  void returnPressed();
  void textChanged(QString newText);

private:
  QStringList history;
  QHash<int, QString> tempHistory;
  qint32 idx;
  TabCompleter *tabCompleter;

  int bindModifier;
  int jumpModifier;

  void resetLine();
  void showHistoryEntry();
};

#endif

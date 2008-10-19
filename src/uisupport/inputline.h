/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef _INPUTLINE_H_
#define _INPUTLINE_H_

#include <QtGui>

class TabCompleter;

class InputLine : public QLineEdit {
  Q_OBJECT

public:
  InputLine(QWidget *parent = 0);
  ~InputLine();
    
protected:
  //    virtual bool event(QEvent *);
  virtual void keyPressEvent(QKeyEvent * event);

private slots:
  void on_returnPressed();
  void on_textChanged(QString newText);

  bool addToHistory(const QString &text, bool temporary = false);

signals:
  void sendText(QString text);

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

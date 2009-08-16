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

#ifndef TABCOMPLETER_H_
#define TABCOMPLETER_H_

#include <QPointer>
#include <QString>
#include <QMap>

#include "types.h"

class MultiLineEdit;
class IrcUser;
class Network;

class TabCompleter : public QObject {
  Q_OBJECT

public:
  explicit TabCompleter(MultiLineEdit *inputLine_);

  void reset();
  void complete();

  virtual bool eventFilter(QObject *obj, QEvent *event);

private:
  struct CompletionKey {
    inline CompletionKey(const QString &n) { nick = n; }
    bool operator<(const CompletionKey &other) const;
    QString nick;
  };

  QPointer<MultiLineEdit> _lineEdit;
  bool _enabled;
  QString _nickSuffix;

  static const Network *_currentNetwork;
  static BufferId _currentBufferId;

  QMap<CompletionKey, QString> _completionMap;
  // QStringList completionTemplates;

  QMap<CompletionKey, QString>::Iterator _nextCompletion;
  int _lastCompletionLength;

  void buildCompletionList();

};

#endif

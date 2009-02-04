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

#ifndef QTUIMESSAGEPROCESSOR_H_
#define QTUIMESSAGEPROCESSOR_H_

#include <QTime>
#include <QTimer>

#include "abstractmessageprocessor.h"

class QtUiMessageProcessor : public AbstractMessageProcessor {
  Q_OBJECT

public:
  enum Mode {
    TimerBased,
    Concurrent
  };

  QtUiMessageProcessor(QObject *parent);

  inline bool isProcessing() const { return _processing; }
  inline Mode processMode() const { return _processMode; }

  void reset();

public slots:
  void process(Message &msg);
  void process(QList<Message> &msgs);

private slots:
  void processNextMessage();
  void nicksCaseSensitiveChanged(const QVariant &variant);
  void highlightListChanged(const QVariant &variant);
  void highlightNickChanged(const QVariant &variant);

private:
  void checkForHighlight(Message &msg);
  void startProcessing();

  QList<QList<Message> > _processQueue;
  QList<Message> _currentBatch;
  QTimer _processTimer;
  bool _processing;
  Mode _processMode;

  struct HighlightRule {
    QString name;
    bool isEnabled;
    Qt::CaseSensitivity caseSensitive;
    bool isRegExp;
    inline HighlightRule(const QString &name, bool enabled, Qt::CaseSensitivity cs, bool regExp)
    : name(name), isEnabled(enabled), caseSensitive(cs), isRegExp(regExp) {}
  };

  QList<HighlightRule> _highlightRules;
  NotificationSettings::HighlightNickType _highlightNick;
  bool _nicksCaseSensitive;
};

#endif

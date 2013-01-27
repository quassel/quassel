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

#ifndef TABCOMPLETER_H_
#define TABCOMPLETER_H_

#include <QPointer>
#include <QString>
#include <QMap>

#include "types.h"

class MultiLineEdit;
class IrcUser;
class Network;

class TabCompleter : public QObject
{
    Q_OBJECT

public:
    enum Type {
        UserTab = 0x01,
        ChannelTab = 0x02
    };

    explicit TabCompleter(MultiLineEdit *inputLine_);

    void reset();
    void complete();

    virtual bool eventFilter(QObject *obj, QEvent *event);

public slots:
    void onTabCompletionKey();

private:

    struct CompletionKey {
        inline CompletionKey(const QString &n) { contents = n; }
        bool operator<(const CompletionKey &other) const;
        QString contents;
    };

    QPointer<MultiLineEdit> _lineEdit;
    bool _enabled;
    QString _nickSuffix;

    static const Network *_currentNetwork;
    static BufferId _currentBufferId;
    static QString _currentBufferName;
    static Type _completionType;

    QMap<CompletionKey, QString> _completionMap;
    // QStringList completionTemplates;

    QMap<CompletionKey, QString>::Iterator _nextCompletion;
    int _lastCompletionLength;

    void buildCompletionList();
};


#endif

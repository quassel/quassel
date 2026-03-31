/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "common-export.h"

#include <QHash>
#include <QMetaMethod>
#include <QObject>
#include <QString>
#include <QStringList>

class COMMON_EXPORT BasicHandler : public QObject
{
    Q_OBJECT

public:
    BasicHandler(QObject* parent = nullptr);
    BasicHandler(QString methodPrefix, QObject* parent = nullptr);

    QStringList providesHandlers();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    using MetaCallArgument = QMetaMethodArgument;
#else
    using MetaCallArgument = QGenericArgument;
#endif

    virtual void handle(const QString& member,
                        MetaCallArgument val0 = {},
                        MetaCallArgument val1 = {},
                        MetaCallArgument val2 = {},
                        MetaCallArgument val3 = {},
                        MetaCallArgument val4 = {},
                        MetaCallArgument val5 = {},
                        MetaCallArgument val6 = {},
                        MetaCallArgument val7 = {},
                        MetaCallArgument val8 = {});

private:
    const QHash<QString, int>& handlerHash();
    QHash<QString, int> _handlerHash;
    int _defaultHandler{-1};
    bool _initDone{false};
    QString _methodPrefix;
};

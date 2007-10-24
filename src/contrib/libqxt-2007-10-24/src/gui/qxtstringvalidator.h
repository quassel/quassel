/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/

#ifndef QXTSTRINGLISTVALIDATOR_H_INCLUDED
#define QXTSTRINGLISTVALIDATOR_H_INCLUDED

#include <QValidator>
#include <QStringList>
#include <QModelIndex>
#include "qxtpimpl.h"
#include "qxtglobal.h"

class QxtStringValidatorPrivate;
class QAbstractItemModel;

class QXT_GUI_EXPORT QxtStringValidator : public QValidator
{
    Q_OBJECT

public:
    QxtStringValidator (QObject * parent);
    ~QxtStringValidator ();

    virtual void fixup ( QString & input ) const;
    virtual QValidator::State validate ( QString & input, int & pos ) const;

    Qt::CaseSensitivity caseSensitivity () const;
    void setCaseSensitivity ( Qt::CaseSensitivity caseSensitivity );
    void setStartModelIndex(const QModelIndex &index);
    void setStringList(const QStringList &stringList);
    void setRecursiveLookup(bool enable);
    void setWrappingLookup(bool enable);
    void setLookupModel(QAbstractItemModel *model);
    void setLookupRole(const int role);

    QModelIndex startModelIndex() const;
    bool recursiveLookup() const;
    bool wrappingLookup() const;
    QAbstractItemModel * lookupModel() const;
    int lookupRole() const;

private:
    QXT_DECLARE_PRIVATE(QxtStringValidator);
};

#endif //QXTSTRINGLISTVALIDATOR_H_INCLUDED

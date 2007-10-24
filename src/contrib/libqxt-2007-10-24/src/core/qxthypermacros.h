/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qt eXTension library
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

/**
\class QxtHyperMacros QxtHyperMacros

\ingroup QxtCore

\brief some helper macros for your daily work

hypermacros use templates in order to work
*/




#ifndef HEADER_GUARDS_QxtHyperMacros_H
#define HEADER_GUARDS_QxtHyperMacros_H


/*! \relates QxtHyperMacros
 * just do something n times
 */
#define fortimes(times) for (QxtHyperValue<typeof(times)> hyperhyperthingyhopefullynooneusesthisnamehere=QxtHyperValue<typeof(times)>(0); hyperhyperthingyhopefullynooneusesthisnamehere<times; hyperhyperthingyhopefullynooneusesthisnamehere++)




template<typename T>
class  QxtHyperValue
{
public:

    QxtHyperValue(const T& p)
    {
        value=p;
    }

    QxtHyperValue<T> & operator=(const QxtHyperValue<T> &rhs)
    {
        value=rhs;
        return this;
    }

    operator T() const
    {
        return value;
    }

    const QxtHyperValue<T>& operator++()
    {
        value++;
        return this;
    }
    const QxtHyperValue<T>& operator--()
    {
        value--;
        return this;
    }
    const QxtHyperValue<T> operator++(int)
    {
        QxtHyperValue<T> clone = *this;
        value++;
        return clone;
    }
    const QxtHyperValue<T> operator--(int)
    {
        QxtHyperValue<T> clone = *this;
        value--;
        return clone;
    }

    bool operator== ( T& v)
    {
        return (value==v);
    }

    bool operator> ( T& v)
    {
        return (value>v);
    }

    bool operator< ( T& v)
    {
        return (value<v);
    }

private:
    T value;
};





#endif

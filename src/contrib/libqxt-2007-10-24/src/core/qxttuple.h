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

#ifndef QXTTUPLE_H
#define QXTTUPLE_H
#include <qxttypelist.h>

namespace QxtType
{

template<typename TYPELIST, int INDEX, int STEP=0, bool END=(INDEX==STEP), bool ERROR=(TYPELIST::length==0)>
struct get
    {
        typedef typename get<typename TYPELIST::tail, INDEX, STEP+1>::type type;
    };

template<typename TYPELIST, int INDEX, int STEP>
struct get<TYPELIST, INDEX, STEP, false, true>
    {}; // does not define type

template<typename TYPELIST, int INDEX, int STEP, bool ERROR>
struct get<TYPELIST, INDEX, STEP, true, ERROR>
    {
        typedef typename TYPELIST::head type;
    };

template<typename TYPELIST, bool LONG=false> class QxtTuple;
template<typename TYPELIST, int INDEX, bool LONG, bool EXT=(INDEX>8)> struct QxtTupleValue;

template<typename TYPELIST, int INDEX> struct QxtTupleValue<TYPELIST, INDEX, true, true>
{
    static typename get<TYPELIST, INDEX>::type value(QxtTuple<TYPELIST,true>* t)
    {
        return QxtTupleValue<typename TYPELIST::extend, INDEX-9, TYPELIST::extend::extends>::value(&t->extend);
    }

    static void setValue(QxtTuple<TYPELIST,true>* t, typename get<TYPELIST, INDEX>::type val)
    {
        QxtTupleValue<typename TYPELIST::extend, INDEX-9, TYPELIST::extend::extends>::setValue(&t->extend, val);
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 0, LONG, false>
{
    static typename get<TYPELIST, 0>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t1;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 0>::type val)
    {
        t->t1 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 1, LONG, false>
{
    static typename get<TYPELIST, 1>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t2;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 1>::type val)
    {
        t->t2 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 2, LONG, false>
{
    static typename get<TYPELIST, 2>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t3;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 2>::type val)
    {
        t->t3 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 3, LONG, false>
{
    static typename get<TYPELIST, 3>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t4;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 3>::type val)
    {
        t->t4 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 4, LONG, false>
{
    static typename get<TYPELIST, 4>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t5;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 4>::type val)
    {
        t->t5 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 5, LONG, false>
{
    static typename get<TYPELIST, 5>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t6;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 5>::type val)
    {
        t->t6 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 6, LONG, false>
{
    static typename get<TYPELIST, 6>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t7;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 6>::type val)
    {
        t->t7 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 7, LONG, false>
{
    static typename get<TYPELIST, 7>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t8;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 7>::type val)
    {
        t->t8 = val;
    }
};

template<typename TYPELIST, bool LONG> struct QxtTupleValue<TYPELIST, 8, LONG, false>
{
    static typename get<TYPELIST, 8>::type value(QxtTuple<TYPELIST,LONG>* t)
    {
        return t->t9;
    }
    static void setValue(QxtTuple<TYPELIST,LONG>* t, typename get<TYPELIST, 8>::type val)
    {
        t->t9 = val;
    }
};

//-----------------------------------------------------------------------------------------------

template<typename TYPELIST>
/**
\class QxtTuple QxtTuple

\ingroup QxtCore

\brief Arbitrary-length templated list

Tuples and cons pairs are both pretty common template metaprogramming hacks. This set of classes
attempts to implement a healthy balance between the two. Tuples generally are implemented with a
fixed length; cons pairs have a lot of overhead and require a ton of typing. As with all template
metaprograms, it may take a while to compile.

It is recommended to use the convenience macros to create tuples, but you can construct the
QxtTypeList template yourself if you desire; this may be preferable if you want to write generic
functions that use tuples.

----- example:
\code
#include <QxtTuple.h>
#include <iostream>
using namespace std;

int main(int argc, char** argv) {
    Qxt7Tuple(bool, char, short, int, long long, float, double) tuple;

    tuple.setValue<0>(true);
    tuple.setValue<1>('a');
    tuple.setValue<2>(32767);
    tuple.setValue<3>(1234567);
    tuple.setValue<4>(987654321);
    tuple.setValue<5>(1.414);
    tuple.setValue<6>(3.14159265);

    cout << 0 << "=" << tuple.value<0>() << endl;
    cout << 1 << "=" << tuple.value<1>() << endl;
    cout << 2 << "=" << tuple.value<2>() << endl;
    cout << 3 << "=" << tuple.value<3>() << endl;
    cout << 4 << "=" << tuple.value<4>() << endl;
    cout << 5 << "=" << tuple.value<5>() << endl;
    cout << 6 << "=" << tuple.value<6>() << endl;
}
\endcode
*/

class QxtTuple<TYPELIST,false>
{
public:
    template<int INDEX> typename get<TYPELIST, INDEX>::type value()
    {
        return QxtTupleValue<TYPELIST, INDEX, false>::value(this);
    }
    template<int INDEX> void setValue(typename get<TYPELIST, INDEX>::type val)
    {
        QxtTupleValue<TYPELIST, INDEX, false>::setValue(this, val);
    }
    bool operator<(const QxtTuple<TYPELIST,false>& other)
    {
        if (t1 < other.t1) return true;
        if (t2 < other.t2) return true;
        if (t3 < other.t3) return true;
        if (t4 < other.t4) return true;
        if (t5 < other.t5) return true;
        if (t6 < other.t6) return true;
        if (t7 < other.t7) return true;
        if (t8 < other.t8) return true;
        if (t9 < other.t9) return true;
        return false;
    }
    bool operator==(const QxtTuple<TYPELIST,false>& other)
    {
        if (t1 != other.t1) return false;
        if (t2 != other.t2) return false;
        if (t3 != other.t3) return false;
        if (t4 != other.t4) return false;
        if (t5 != other.t5) return false;
        if (t6 != other.t6) return false;
        if (t7 != other.t7) return false;
        if (t8 != other.t8) return false;
        if (t9 != other.t9) return false;
        return true;
    }
    bool operator>=(const QxtTuple<TYPELIST,false>& other)
    {
        return !(*this < other);
    }
    bool operator<=(const QxtTuple<TYPELIST,false>& other)
    {
        if (t1 <= other.t1) return true;
        if (t2 <= other.t2) return true;
        if (t3 <= other.t3) return true;
        if (t4 <= other.t4) return true;
        if (t5 <= other.t5) return true;
        if (t6 <= other.t6) return true;
        if (t7 <= other.t7) return true;
        if (t8 <= other.t8) return true;
        if (t9 <= other.t9) return true;
        return false;
    }
    bool operator>(const QxtTuple<TYPELIST,false>& other)
    {
        return !(*this <= other);
    }
    bool operator!=(const QxtTuple<TYPELIST,false>& other)
    {
        return !(*this == other);
    }


// if only we could get away with making these protected
    typename get<TYPELIST, 0>::type t1;
    typename get<TYPELIST, 1>::type t2;
    typename get<TYPELIST, 2>::type t3;
    typename get<TYPELIST, 3>::type t4;
    typename get<TYPELIST, 4>::type t5;
    typename get<TYPELIST, 5>::type t6;
    typename get<TYPELIST, 6>::type t7;
    typename get<TYPELIST, 7>::type t8;
    typename get<TYPELIST, 8>::type t9;
};

//-----------------------------------------------------------------------------------------------

template<typename TYPELIST>
class QxtTuple<TYPELIST,true>
{
public:
    template<int INDEX> typename get<TYPELIST, INDEX>::type value()
    {
        return QxtTupleValue<TYPELIST, INDEX, true>::value(this);
    }
    template<int INDEX> void setValue(typename get<TYPELIST, INDEX>::type val)
    {
        QxtTupleValue<TYPELIST, INDEX, true>::setValue(this, val);
    }
    bool operator<(const QxtTuple<TYPELIST,true>& other)
    {
        if (t1 < other.t1) return true;
        if (t2 < other.t2) return true;
        if (t3 < other.t3) return true;
        if (t4 < other.t4) return true;
        if (t5 < other.t5) return true;
        if (t6 < other.t6) return true;
        if (t7 < other.t7) return true;
        if (t8 < other.t8) return true;
        if (t9 < other.t9) return true;
        if (extend < other.extend) return true;
        return false;
    }
    bool operator==(const QxtTuple<TYPELIST,true>& other)
    {
        if (t1 != other.t1) return false;
        if (t2 != other.t2) return false;
        if (t3 != other.t3) return false;
        if (t4 != other.t4) return false;
        if (t5 != other.t5) return false;
        if (t6 != other.t6) return false;
        if (t7 != other.t7) return false;
        if (t8 != other.t8) return false;
        if (t9 != other.t9) return false;
        if (extend != other.extend) return false;
        return true;
    }
    bool operator>=(const QxtTuple<TYPELIST,true>& other)
    {
        return !(*this < other);
    }
    bool operator<=(const QxtTuple<TYPELIST,true>& other)
    {
        if (t1 <= other.t1) return true;
        if (t2 <= other.t2) return true;
        if (t3 <= other.t3) return true;
        if (t4 <= other.t4) return true;
        if (t5 <= other.t5) return true;
        if (t6 <= other.t6) return true;
        if (t7 <= other.t7) return true;
        if (t8 <= other.t8) return true;
        if (t9 <= other.t9) return true;
        if (extend <= other.extend) return true;
        return false;
    }
    bool operator>(const QxtTuple<TYPELIST,true>& other)
    {
        return !(*this <= other);
    }
    bool operator!=(const QxtTuple<TYPELIST,true>& other)
    {
        return !(*this == other);
    }

// if only we could get away with making these protected
    typename get<TYPELIST, 0>::type t1;
    typename get<TYPELIST, 1>::type t2;
    typename get<TYPELIST, 2>::type t3;
    typename get<TYPELIST, 3>::type t4;
    typename get<TYPELIST, 4>::type t5;
    typename get<TYPELIST, 5>::type t6;
    typename get<TYPELIST, 6>::type t7;
    typename get<TYPELIST, 7>::type t8;
    typename get<TYPELIST, 8>::type t9;
    QxtTuple<typename TYPELIST::extend> extend;
};

}

#ifndef QXT_NO_USING
using QxtType::QxtTuple;
#endif

#ifndef QXT_NO_MACROS
/*! \relates QxtTuple
 * Declares a one-column tuple.
 */
#define Qxt1Tuple(a) QxtTuple<QxtTypeList<a > >

/*! \relates QxtTuple
 * Declares a two-column tuple, similar to QPair.
 */
#define Qxt2Tuple(a, b) QxtTuple<QxtTypeList<a, b > >

/*! \relates QxtTuple
 * Declares a three-column tuple, similar to QxtTriple.
 */
#define Qxt3Tuple(a, b, c) QxtTuple<QxtTypeList<a, b, c > >

/*! \relates QxtTuple
 * Declares a four-column tuple.
 */
#define Qxt4Tuple(a, b, c, d) QxtTuple<QxtTypeList<a, b, c, d > >

/*! \relates QxtTuple
 * Declares a five-column tuple.
 */
#define Qxt5Tuple(a, b, c, d, e) QxtTuple<QxtTypeList<a, b, c, d, e > >

/*! \relates QxtTuple
 * Declares a six-column tuple.
 */
#define Qxt6Tuple(a, b, c, d, e, f) QxtTuple<QxtTypeList<a, b, c, d, e, f > >

/*! \relates QxtTuple
 * Declares a seven-column tuple.
 */
#define Qxt7Tuple(a, b, c, d, e, f, g) QxtTuple<QxtTypeList<a, b, c, d, e, f, g > >

/*! \relates QxtTuple
 * Declares an eight-column tuple.
 */
#define Qxt8Tuple(a, b, c, d, e, f, g, h) QxtTuple<QxtTypeList<a, b, c, d, e, f, g, h > >

/*! \relates QxtTuple
 * Declares a nine-column tuple.
 */
#define Qxt9Tuple(a, b, c, d, e, f, g, h, i) QxtTuple<QxtTypeList<a, b, c, d, e, f, g, h, i > >

/*! \relates QxtTuple
 * Declares an extended tuple with ten or more columns. Pay special attention to the syntax of the tenth parameter, which
 * must be a QxtTypeList, not a storage type.
\code
QxtLongTuple(int, int, int, int, int, int, int, int, int, Qxt1TypeList(int)) tuple; // correct way to implement a 10-tuple
QxtLongTuple(int, int, int, int, int, int, int, int, int, int) tuple;              // this will produce a (very long) compile-time error
\endcode
 */
#define QxtLongTuple(a, b, c, d, e, f, g, h, i, extend) QxtTuple<QxtTypeList<a, b, c, d, e, f, g, h, i, extend >, true >

#endif

#endif

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
\class QxtPairList QxtPairList

\ingroup kit

\brief Searchable List of Pairs


Pair list provides a list with two values, a bit like QHash, but with posibility to operate on both values.

in contrast to QHash, every entry has a unique id, you can work with.  like a QList.

\code
QxtPairList<int,int> list;

list.append(1,2);
list.append(1,5);
list.append(5,6);

qDebug()<< list.find(1);    //  "0,1"
qDebug()<< list.find(SKIP,5);    //  "2"
qDebug()<< list.find(5);    //  "2"

\endcode
you may allso play around with the list itself

\code
list.list.append(qMakePair(1,2));
\endcode


*/


#include <QList>
#include <QPair>
#include <QxtNullable>
#include <qxtglobal.h>



template <typename T, typename K>
class QXT_CORE_EXPORT QxtPairList
{


public:

    QxtPairList()
    {}

    QxtPairList( const QxtPairList<T,K> & other )
    {
        list= other.list;
    }

    QxtPairList operator= ( const QxtPairList<T,K> & other )
    {
        list= other.list;
    }


    void append(T v1, K v2)
    {
        list.append(qMakePair(v1,v2));
    }


    /** \brief search entrys by match

    both arguments are optional, due to the use of QxtNullable

    \code
    find(SKIP,v2);
    find(v1,SKIP);
    find(v1);
    \endcode
    are all valid
    */

    QList<int> find( qxNull(T,v1) , qxNull(K,v2) )
    {
        QList<int> found;

        if ((!v1.isNull()) and (!v2.isNull()))
        {
            for (int i=0;i<list.count();i++)
                if ((list[i].first()==v1)and(list[i].second()==v2))
                    found.append(i);

            return found;
        }

        if ((!v1.isNull()) and (v2.isNull()))
        {
            for (int i=0;i<list.count();i++)
                if (list[i].first()==v1)
                    found.append(i);

            return found;
        }

        if ((v1.isNull()) and (!v2.isNull()))
        {
            for (int i=0;i<list.count();i++)
                if (list[i].second()==v2)
                    found.append(i);

            return found;
        }



    }



    ///remove an entrys position by position
    void remove(int nr)
    {
        list.removeAt(nr);
    }

    ///remove a list of entrys by position
    void remove(QList<int> nrs)
    {
        int i;
        foreach (i,nrs)
        list.removeAt(i);
    }




    /** \brief operate on the list directly

    you may use the internal list directly, but be carefull 
    dont expect to work the QxPairList to work normal if you mess around with it.		
    */


    QList<QPair<T,K> > list;
};


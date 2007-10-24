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



#ifndef QXTERROR_H
#define QXTERROR_H
#include <qxtglobal.h>
#include <qxtnamespace.h>

/**
\class QxtError QxtError

\ingroup QxtCore

\brief Information about Errors ocuring inside Qxt

*/

/*! \relates QxtError
droping an error inside a function that returns QxtError


short for return  QxtError(__FILE__,__LINE__,x);
*/
#define QXT_DROP(x) return QxtError(__FILE__,__LINE__,x);


/*! \relates QxtError
droping an error inside a function that returns QxtError

aditionaly specifies an errorstring \n

short for return  QxtError(__FILE__,__LINE__,x,s);
*/
#define QXT_DROP_S(x,s) return QxtError(__FILE__,__LINE__,x,s);


/*! \relates QxtError
droping no error inside a function that returns QxtError

short for return QxtError(__FILE__,__LINE__,Qxt::NoError);
*/
#define QXT_DROP_OK return QxtError(__FILE__,__LINE__,Qxt::NoError);


/*! \relates QxtError
forward a drop


drops from this function if the call inside dropped too.
the inner function must return or be a QxtError.

example
\code
QXT_DROP_F(critical_function());
\endcode

*/
#define QXT_DROP_F(call) {QxtError error_sds = call; if (error_sds != Qxt::NoError ) return error_sds; else (void)0; }

/*! \relates QxtError
check for errors

example
\code
QXT_DROP_SCOPE(error,critical_function())
	{
	qDebug()<<error;
	QXT_DROP_F(error);
	};
\endcode

short for  QxtError name = call; if (name != Qxt::NoError )

\warning: the errors name is valid outside the scope
*/
#define QXT_DROP_SCOPE(name,call) QxtError name = call; if (name != Qxt::NoError )






class QXT_CORE_EXPORT QxtError
{
public:
    QxtError(const char * file, long line, Qxt::ErrorCode errorcode, const char * errorString=0);
    Qxt::ErrorCode errorCode() const;
    long line() const;
    const char * file() const;
    const char * errorString() const;
    operator Qxt::ErrorCode();


private:
    Qxt::ErrorCode errorcode_m;
    long line_m;
    const char * file_m;
    const char * errorString_m;
};

#endif

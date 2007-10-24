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
#include "qxterror.h"

QxtError::QxtError(const char * file, long line, Qxt::ErrorCode errorcode,const char * errorString)
{
    file_m=file;
    line_m=line;
    errorcode_m=errorcode;
    errorString_m=errorString;
}


Qxt::ErrorCode QxtError::errorCode() const
{
    return errorcode_m;
}

long QxtError::line() const
{
    return line_m;
}

const char * QxtError::file() const
{
    return file_m;
}

QxtError::operator Qxt::ErrorCode()
{
    return errorcode_m;
}
/*!
The Error String or NULL
depending how the error was constructed.
Be carefull with stack and temporary objects, QxtError just saves the pointer you passed, not the actual data.
*/
const char * QxtError::errorString() const
{
    return errorString_m;
}



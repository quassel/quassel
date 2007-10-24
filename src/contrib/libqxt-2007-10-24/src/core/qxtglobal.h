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


/** \defgroup QxtCore QxtCore
        The Core contains naturaly  the most important parts of Qxt, everything is based on.
*/

/** \defgroup QxtSql QxtSql
        parts that depend on QtSql (and at least QtCore)
*/

/** \defgroup QxtNetwork  QxtNetwork
        parts that depend on QtNetwork (and at least QtCore)
*/

/** \defgroup QxtGui QxtGui
        parts that depend on QtGui (and at least QtCore)
*/

/** \defgroup QxtWeb  QxtWeb
        Qt like Web Developement tools
*/

/** \defgroup QxtCrypto  QxtCrypto
        Tools for encryption and hashing
*/



/**

*/
/*! \mainpage Qt extension librarys
 *
 *
 * Qxt Provides Classes for extended needs based on the Qt Library by Trolltech
 *
 * Checkout the classes list

\image html logo.png
 *
 */


#ifndef QXTDEFINES_HEADER_GUARDD
#define QXTDEFINES_HEADER_GUARDD





#define QXT_VERSION (  (2<<16) + (2<<8) + 3 )



//--------------------------global macros------------------------------

#ifndef QXT_NO_MACROS

#endif

//--------------------------export macros------------------------------




#    define QXT_DLLEXPORT DO_NOT_USE_THIS_ANYMORE







#if defined(QXT_WINDOWS)

#	if defined(BUILD_QXT_CORE)
#   		define QXT_CORE_EXPORT __declspec(dllexport)
#	else
#   		define QXT_CORE_EXPORT __declspec(dllimport)
#	endif

#	if defined(BUILD_QXT_GUI)
#   		define QXT_GUI_EXPORT __declspec(dllexport)
#	else
#   		define QXT_GUI_EXPORT __declspec(dllimport)
#	endif

#	if defined(BUILD_QXT_MEDIA)
#   		define QXT_MEDIA_EXPORT __declspec(dllexport)
#	else
#   		define QXT_MEDIA_EXPORT __declspec(dllimport)
#	endif


#	if defined(BUILD_QXT_NETWORK)
#   		define QXT_NETWORK_EXPORT __declspec(dllexport)
#	else
#   		define QXT_NETWORK_EXPORT __declspec(dllimport)
#	endif


#	if defined(BUILD_QXT_SQL)
#   		define QXT_SQL_EXPORT __declspec(dllexport)
#	else
#   		define QXT_SQL_EXPORT __declspec(dllimport)
#	endif

#	if defined(BUILD_QXT_WEB)
#   		define QXT_WEB_EXPORT __declspec(dllexport)
#	else
#   		define QXT_WEB_EXPORT __declspec(dllimport)
#	endif

#	if defined(BUILD_QXT_CRYPTO)
#   		define QXT_CRYPTO_EXPORT __declspec(dllexport)
#	else
#   		define QXT_CRYPTO_EXPORT __declspec(dllimport)
#	endif


#else

#    define QXT_CORE_EXPORT
#    define QXT_GUI_EXPORT
#    define QXT_MEDIA_EXPORT
#    define QXT_NETWORK_EXPORT
#    define QXT_SQL_EXPORT
#    define QXT_WEB_EXPORT
#    define QXT_CRYPTO_EXPORT




#endif










#if defined BUILD_QXT_CORE ||  defined BUILD_QXT_GUI || defined BUILD_QXT_MEDIA  || defined  BUILD_QXT_SQL || defined BUILD_QXT_NETWORK || defined BUILD_QXT_KIT || defined BUILD_QXT_WEB || defined BUILD_QXT_CRYPTO
#define BUILD_QXT
#endif











#endif //guards


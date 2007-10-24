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
#include "qxtsemaphore.h"

/**
\class QxtSemaphore QxtSemaphore
\ingroup QxtCore
\brief system wide semaphore (former QxtSingleInstance)


\code
	QxtSemaphore instance("com.mycompany.foobla.uniquestring");
	if(!instance.trylock())
		{
		qDebug("already started")
		}

\endcode

Note that the semaphore is autoaticly unlocked on destruction, but not on segfault,sigkill,etc...!
*/

#ifdef Q_WS_WIN
#include "Windows.h"

class QxtSemaphorePrivate : public QxtPrivate<QxtSemaphore>
{
public:
    QString name;
    unsigned sem_m;
    void init()
    {
        sem_m=0;
    }

    bool trylock()
    {
        sem_m = (unsigned ) CreateSemaphoreA ( NULL , 1 , 2 , qPrintable("Global\\"+name) );
        if (sem_m == 0 )
            return false;
        return true;
    }
    bool unlock()
    {
        if (sem_m==0)
            return false;
        return CloseHandle((void *)sem_m);
    }
};



#else

#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

class QxtSemaphorePrivate : public QxtPrivate<QxtSemaphore>
{
public:
    QString name;
    sem_t* m_sem;
    bool s_N;
    void init()
    {
        s_N=false;
        m_sem=NULL;
    }

    bool trylock()
    {
        m_sem=sem_open(qPrintable(name), O_CREAT, S_IRUSR | S_IWUSR, 1);
        if (m_sem==(sem_t*)(SEM_FAILED) || sem_trywait(m_sem))
        {
            m_sem=NULL;
            s_N=true;
            return false;
        }
        s_N=false;
        return true;
    }
    bool unlock()
    {
        if (m_sem==NULL)
            return false;
        if (!s_N)
        {
            sem_post(m_sem);
        }
        return (sem_close(m_sem)==0);
    }
};

#endif

QxtSemaphore::QxtSemaphore(QString uniqueID)
{
    qxt_d().name=uniqueID;
    qxt_d().init();
}

QxtSemaphore::~QxtSemaphore()
{
    unlock();
}

bool QxtSemaphore::trylock()
{
    return qxt_d().trylock();
}
bool QxtSemaphore::unlock()
{
    return qxt_d().unlock();
}

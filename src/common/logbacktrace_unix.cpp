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

#include "quassel.h"

#if defined(HAVE_EXECINFO) && !defined(Q_OS_MAC)
#  define BUILD_CRASHHANDLER
#  include <execinfo.h>
#  include <dlfcn.h>
#  include <cxxabi.h>
#  include <QFile>
#  include <QTextStream>
#  include <QDebug>
#endif

void Quassel::logBacktrace(const QString &filename)
{
#ifndef BUILD_CRASHHANDLER
    Q_UNUSED(filename)
#else
    void *callstack[128];
    int i, frames = backtrace(callstack, 128);

    QFile dumpFile(filename);
    dumpFile.open(QIODevice::Append);
    QTextStream dumpStream(&dumpFile);

    for (i = 0; i < frames; ++i) {
        Dl_info info;
        dladdr(callstack[i], &info);
        // as a reference:
        //     typedef struct
        //     {
        //       __const char *dli_fname;   /* File name of defining object.  */
        //       void *dli_fbase;           /* Load address of that object.  */
        //       __const char *dli_sname;   /* Name of nearest symbol.  */
        //       void *dli_saddr;           /* Exact value of nearest symbol.  */
        //     } Dl_info;

    #if __LP64__
        int addrSize = 16;
    #else
        int addrSize = 8;
    #endif

        QString funcName;
        if (info.dli_sname) {
            char *func = abi::__cxa_demangle(info.dli_sname, 0, 0, 0);
            if (func) {
                funcName = QString(func);
                free(func);
            }
            else {
                funcName = QString(info.dli_sname);
            }
        }
        else {
            funcName = QString("0x%1").arg((ulong)(info.dli_saddr), addrSize, 16, QLatin1Char('0'));
        }

        // prettificating the filename
        QString fileName("???");
        if (info.dli_fname) {
            fileName = QString(info.dli_fname);
            int slashPos = fileName.lastIndexOf('/');
            if (slashPos != -1)
                fileName = fileName.mid(slashPos + 1);
        }

        QString debugLine = QString("#%1 %2 0x%3 %4").arg(i, 3, 10)
                            .arg(fileName, -20)
                            .arg((ulong)(callstack[i]), addrSize, 16, QLatin1Char('0'))
                            .arg(funcName);

        dumpStream << debugLine << "\n";
        qDebug() << qPrintable(debugLine);
    }
    dumpFile.close();
#endif /* BUILD_CRASHHANDLER */
}

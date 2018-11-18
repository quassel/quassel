/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <cstdio>

#include <windows.h>
#include <dbghelp.h>

#include <QFile>
#include <QTextStream>

#include "quassel.h"

void loadHelpStackFrame(IMAGEHLP_STACK_FRAME& ihsf, const STACKFRAME64& stackFrame)
{
    ZeroMemory(&ihsf, sizeof(IMAGEHLP_STACK_FRAME));
    ihsf.InstructionOffset = stackFrame.AddrPC.Offset;
    ihsf.FrameOffset = stackFrame.AddrFrame.Offset;
}

BOOL CALLBACK EnumSymbolsCB(PSYMBOL_INFO symInfo, ULONG size, PVOID user)
{
    Q_UNUSED(size)
    QStringList* params = (QStringList*)user;
    if (symInfo->Flags & SYMFLAG_PARAMETER) {
        params->append(symInfo->Name);
    }
    return TRUE;
}

struct EnumModulesContext
{
    HANDLE hProcess;
    QTextStream& stream;
    EnumModulesContext(HANDLE hProcess, QTextStream& stream)
        : hProcess(hProcess)
        , stream(stream)
    {}
};

BOOL CALLBACK EnumModulesCB(LPCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
{
    Q_UNUSED(ModuleName)
    IMAGEHLP_MODULE64 mod;
    EnumModulesContext* context = (EnumModulesContext*)UserContext;
    mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    if (SymGetModuleInfo64(context->hProcess, BaseOfDll, &mod)) {
        QString line = QString("%1 0x%2 Image: %3").arg(mod.ModuleName, -14).arg(BaseOfDll, 8, 16, QLatin1Char('0')).arg(mod.LoadedImageName);
        // qDebug() << qPrintable(line);
        context->stream << line << '\n';

        QString pdbName(mod.LoadedPdbName);
        if (!pdbName.isEmpty()) {
            QString line2 = QString("%1 %2").arg("", 32).arg(pdbName);
            // qDebug() << qPrintable(line2);
            context->stream << line2 << '\n';
        }
    }
    return TRUE;
}

#if defined(_M_IX86) && defined(Q_CC_MSVC)
// Disable global optimization and ignore /GS waning caused by
// inline assembly.
// not needed with mingw cause we can tell mingw which registers we use
#    pragma optimize("g", off)
#    pragma warning(push)
#    pragma warning(disable : 4748)
#endif
void Quassel::logBacktrace(const QString& filename)
{
    DWORD MachineType;
    CONTEXT Context;
    STACKFRAME64 StackFrame;

#ifdef _M_IX86
    ZeroMemory(&Context, sizeof(CONTEXT));
    Context.ContextFlags = CONTEXT_CONTROL;

#    ifdef __MINGW32__
    asm("Label:\n\t"
        "movl %%ebp,%0;\n\t"
        "movl %%esp,%1;\n\t"
        "movl $Label,%%eax;\n\t"
        "movl %%eax,%2;\n\t"
        : "=r"(Context.Ebp), "=r"(Context.Esp), "=r"(Context.Eip)
        :  // no input
        : "eax");
#    else
    _asm {
Label:
        mov[Context.Ebp], ebp;
        mov[Context.Esp], esp;
        mov eax, [Label];
        mov[Context.Eip], eax;
    }
#    endif
#else
    RtlCaptureContext(&Context);
#endif

    ZeroMemory(&StackFrame, sizeof(STACKFRAME64));
#ifdef _M_IX86
    MachineType = IMAGE_FILE_MACHINE_I386;
    StackFrame.AddrPC.Offset = Context.Eip;
    StackFrame.AddrPC.Mode = AddrModeFlat;
    StackFrame.AddrFrame.Offset = Context.Ebp;
    StackFrame.AddrFrame.Mode = AddrModeFlat;
    StackFrame.AddrStack.Offset = Context.Esp;
    StackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_X64)
    MachineType = IMAGE_FILE_MACHINE_AMD64;
    StackFrame.AddrPC.Offset = Context.Rip;
    StackFrame.AddrPC.Mode = AddrModeFlat;
    StackFrame.AddrFrame.Offset = Context.Rsp;
    StackFrame.AddrFrame.Mode = AddrModeFlat;
    StackFrame.AddrStack.Offset = Context.Rsp;
    StackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_IA64)
    MachineType = IMAGE_FILE_MACHINE_IA64;
    StackFrame.AddrPC.Offset = Context.StIIP;
    StackFrame.AddrPC.Mode = AddrModeFlat;
    StackFrame.AddrFrame.Offset = Context.IntSp;
    StackFrame.AddrFrame.Mode = AddrModeFlat;
    StackFrame.AddrBStore.Offset = Context.RsBSP;
    StackFrame.AddrBStore.Mode = AddrModeFlat;
    StackFrame.AddrStack.Offset = Context.IntSp;
    StackFrame.AddrStack.Mode = AddrModeFlat;
#else
#    error "Unsupported platform"
#endif

    // EnterCriticalSection(&DbgHelpLock);

    QFile logFile(filename);
    logFile.open(QIODevice::Append);
    QTextStream logStream(&logFile);

    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();
    SymInitialize(hProcess, NULL, TRUE);

    DWORD64 dwDisplacement;

    ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    IMAGEHLP_MODULE64 mod;
    mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

    IMAGEHLP_STACK_FRAME ihsf;
    ZeroMemory(&ihsf, sizeof(IMAGEHLP_STACK_FRAME));

    int i = 0;
    while (StackWalk64(MachineType, hProcess, hThread, &StackFrame, &Context, NULL, NULL, NULL, NULL)) {
        if (i == 128)
            break;

        loadHelpStackFrame(ihsf, StackFrame);
        if (StackFrame.AddrPC.Offset != 0) {  // Valid frame.
            QString fileName("???");
            if (SymGetModuleInfo64(hProcess, ihsf.InstructionOffset, &mod)) {
                fileName = QString(mod.ImageName);
                int slashPos = fileName.lastIndexOf('\\');
                if (slashPos != -1)
                    fileName = fileName.mid(slashPos + 1);
            }
            QString funcName;
            if (SymFromAddr(hProcess, ihsf.InstructionOffset, &dwDisplacement, pSymbol)) {
                funcName = QString(pSymbol->Name);
            }
            else {
                funcName = QString("0x%1").arg(ihsf.InstructionOffset, 8, 16, QLatin1Char('0'));
            }
            QStringList params;
            SymSetContext(hProcess, &ihsf, NULL);
            SymEnumSymbols(hProcess, 0, NULL, EnumSymbolsCB, (PVOID)&params);

            QString debugLine = QString("#%1 %2 0x%3 %4(%5)")
                                    .arg(i, 3, 10)
                                    .arg(fileName, -20)
                                    .arg(ihsf.InstructionOffset, 8, 16, QLatin1Char('0'))
                                    .arg(funcName)
                                    .arg(params.join(", "));
            // qDebug() << qPrintable(debugLine);
            logStream << debugLine << '\n';
            i++;
        }
        else {
            break;  // we're at the end.
        }
    }

    // qDebug() << "List of linked Modules:";
    logStream << "\n\nList of linked Modules:\n";
    EnumModulesContext modulesContext(hProcess, logStream);
    SymEnumerateModules64(hProcess, EnumModulesCB, (PVOID)&modulesContext);

    logFile.close();
}

#if defined(_M_IX86) && defined(Q_CC_MSVC)
#    pragma warning(pop)
#    pragma optimize("g", on)
#endif

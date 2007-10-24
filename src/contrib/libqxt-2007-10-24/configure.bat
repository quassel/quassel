@ECHO off
SET QMAKE=qmake
SET MAKE=nmake
SET PROJECT_ROOT=%CD%
SET TESTDIR=%PROJECT_ROOT%\config.tests
SET CONFIG_LOG=config.log
SET LAST_FUNC_RET=0

if exist %PROJECT_ROOT%\config.in   del %PROJECT_ROOT%\config.in
if exist %PROJECT_ROOT%\config.log  del %PROJECT_ROOT%\config.log
if exist %PROJECT_ROOT%\depends.pri del %PROJECT_ROOT%\depends.pri

echo #depends.pri > %PROJECT_ROOT%\depends.pri
    
shift
set QMAKEBIN=qmake
set MSVCMODE=
set OPENSSL=1
set FCGI=1
echo include(depends.pri) > %PROJECT_ROOT%\config.in
echo QXT_stability += unknown >> %PROJECT_ROOT%\config.in

:top
if "%0" == ""            goto finish
if "%0" == "-qmake-bin"  goto setqmake
if "%0" == "-I"          goto addinclude 
if "%0" == "-L"          goto addlibpath
if "%0" == "-l"          goto addlib
if "%0" == "-nomake"     goto nomake
if "%0" == "-prefix"     goto prefix
if "%0" == "-libdir"     goto libdir 
if "%0" == "-docdir"     goto docdir
if "%0" == "-headerdir"  goto headerdir
if "%0" == "-bindir"     goto bindir
if "%0" == "-static"     goto static
if "%0" == "-debug"      goto debug
if "%0" == "-release"    goto release
if "%0" == "-no-openssl" goto noopenssl
if "%0" == "-msvc"       goto msvc
if "%0" == "/help"       goto help
if "%0" == "-help"       goto help
if "%0" == "--help"      goto help
if "%0" == "/?"          goto help

echo Unrecognized configure option: %0
del %PROJECT_ROOT%\config.in
goto end

:setqmake
set QMAKEBIN=%1
goto bottom2

:addinclude
echo INCLUDEPATH += "%1" >> %PROJECT_ROOT%\depends.pri
goto bottom2	

:addlibpath
echo LIBS += -L"%1" >> %PROJECT_ROOT%\depends.pri
goto bottom2

:addlib
echo LIBS += -l"%1" >> %PROJECT_ROOT%\depends.pri
goto bottom2

:nomake
if "%1"=="fcgi" set FCGI=0
echo QXT_BUILD -= %1 >> %PROJECT_ROOT%\config.in
goto bottom2

:prefix
echo QXTINSTALLDIR = %1 >> %PROJECT_ROOT%\config.in
goto bottom2

:libdir
echo target.path = %1 >> %PROJECT_ROOT%\config.in
goto bottom2

:docdir
echo docs.path = %1 >> %PROJECT_ROOT%\config.in
goto bottom2

:headerdir
echo include.path = %1 >> %PROJECT_ROOT%\config.in
goto bottom2

:bindir
echo bin.path = %1 >> %PROJECT_ROOT%\config.in
goto bottom2

:static
echo CONFIG += static staticlib >> %PROJECT_ROOT%\config.in
goto bottom    

:debug
echo CONFIG += debug >> %PROJECT_ROOT%\config.in
goto bottom

:release
echo CONFIG += release >> %PROJECT_ROOT%\config.in
goto bottom

:noopenssl
set OPENSSL=0
echo QXT_LIBS -= openssl >> %PROJECT_ROOT%\config.in
goto bottom

:msvc
set MSVCMODE=-tp vc
goto bottom

:bottom2
    shift
:bottom
    shift
goto top
    
:help
    echo Usage: configure [-prefix (dir)] [-libdir (dir)] [-docdir (dir)]
    echo        [-bindir (dir)] [-headerdir (dir)] [-qmake-bin (path)]
    echo        [-static] [-debug] [-release] [-no-stability-unknown]
    echo        [-no-openssl] [-nomake (module)] [-msvc]
    echo.
    echo Installation options:
    echo.
    echo -prefix (dir) ....... This will install everything relative to (dir)
    echo                       default: C:\libqxt
    echo -libdir (dir) ....... Libraries will be installed to (dir)
    echo                       default: PREFIX/lib
    echo -docdir (dir) ....... Documentation will be installed to (dir)
    echo                       default: PREFIX/share/doc
    echo -bindir (dir) ....... Executables will be installed to (dir)
    echo                       default: PREFIX/bin
    echo -headerdir (dir) .... Include files will be installed to (dir)
    echo                       default: PREFIX/include
    echo -qmake-bin (path) ... Specifies the path to the qmake executable
    echo                       default: search the system path
    echo -L (path)............ Specifies the a additional library search path
    echo -I (path)............ Specifies the a additional include search path
    echo -l (path)............ Add a custom library
    echo -static ............. Compile Qxt as a static library
    echo -debug .............. Build Qxt with debugging symbols
    echo -release ............ Build Qxt without debugging support
    echo -no-openssl ......... Do not link to OpenSSL
    echo -nomake (module) .... Do not compile the specified module
    echo                       options: network gui sql media web designer
    echo -msvc ............... Configure Qxt to use Microsoft Visual Studio

    del %PROJECT_ROOT%\config.in
    goto end

:finish
echo    Testing for qmake...
qmake -v >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto qmakeERR
goto qmakeOK
:qmakeERR
echo    You don't seem to have 'qmake' in your PATH.
echo    Cannot proceed.
goto end

:qmakeOK
if "%QMAKESPEC%" == "win32-msvc"     goto testnmake
if "%QMAKESPEC%" == "win32-msvc.net" goto testnmake
if "%QMAKESPEC%" == "win32-msvc2005" goto testnmake

:testmingw
echo    Testing for mingw32-make...
call mingw32-make -v >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto testnmake
echo        Using mingw32-make. 
SET MAKE=mingw32-make
GOTO detectTools_end_test_make     

:testnmake
if "%QMAKESPEC%" == "win32-g++"     goto testgmake
echo    Testing for nmake...
call nmake /? >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto testgmake
echo        Using nmake.
SET MAKE=nmake
GOTO detectTools_end_test_make

:testgmake
echo    Testing for GNU make...
call make -v >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto missingmake
echo        Using GNU make.
SET MAKE=make
GOTO detectTools_end_test_make
 
:missingmake
echo    You don't seem to have 'mingw32-make', 'nmake' or 'make' in your PATH. 
echo    Cannot proceed.
goto end

:detectTools_end_test_make
if "%OPENSSL%"=="0" goto detectfcgi
echo    Testing for OpenSSL... 
echo OpenSSL... >> %PROJECT_ROOT%\%CONFIG_LOG%
cd %TESTDIR%\openssl
%QMAKE% >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto opensslfailed
call %MAKE% clean >> %PROJECT_ROOT%\%CONFIG_LOG%
call %MAKE% >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto opensslfailed
set OPENSSL=1
goto detectfcgi

:opensslfailed
set OPENSSL=0
echo QXT_LIBS -= openssl >> %PROJECT_ROOT%\config.in

:detectfcgi
if "%FCGI%"=="0" goto skipfcgitest
echo    Testing for FastCGI...
echo fcgi... >> %PROJECT_ROOT%\%CONFIG_LOG%
cd %TESTDIR%\fcgi
%QMAKE% >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto fcgifailed
call %MAKE% clean >> %PROJECT_ROOT%\%CONFIG_LOG%
call %MAKE% >> %PROJECT_ROOT%\%CONFIG_LOG%
if errorlevel 1 goto fcgifailed
set FCGI=1
goto alltestsok

:fcgifailed
set FCGI=0
echo QXT_BUILD -= fcgi >> %PROJECT_ROOT%\config.in

:skipfcgitest
:alltestsok
cd %PROJECT_ROOT%

echo    Configuration successful.
if "%OPENSSL%"=="1" echo        OpenSSL enabled.
if "%OPENSSL%"=="0" echo        OpenSSL disabled.
if "%FCGI%"=="1"    echo        FastCGI enabled.
if "%FCGI%"=="0"    echo        FastCGI disabled.
echo    Generating makefiles...
copy %PROJECT_ROOT%\config.pri %PROJECT_ROOT%\config.pri.bak >> %PROJECT_ROOT%\%CONFIG_LOG%
copy %PROJECT_ROOT%\config.in %PROJECT_ROOT%\config.pri >> %PROJECT_ROOT%\%CONFIG_LOG%
del %PROJECT_ROOT%\config.in >> %PROJECT_ROOT%\%CONFIG_LOG%
%QMAKE% %MSVCMODE% -recursive
if errorlevel 1 goto mainqmakeERR

echo    Makefiles generated. Run %MAKE% now.
goto end

:mainqmakeERR
echo    Error running qmake. Cannot proceed.
goto end

:end 

@echo off

rem This is needed to run cmake properly on my system.
rem There are some programs in my $PATH, that inhibit cmake from working correctly.
rem -- kaffeedoktor

if exist win_prepare.bat CALL win_prepare.bat

rem Build the whole project
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
cd ..
	
if exist win_restore.bat CALL win_restore.bat

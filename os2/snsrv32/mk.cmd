@echo off
nmake /f snsrv_32.mak %1 %2 %3 >error.log
if errorlevel 1 goto error
goto ende
:error
t error.log
:ende

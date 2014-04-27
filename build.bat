@echo off
@set CC65_HOME=..\CA65Compiler\
@if "%PATH%"=="%PATH:cc65=%" @PATH=%PATH%;%CC65_HOME%bin\

call make clean
call make

REM you must set a default program to open nes file
DwarvesManager.nes
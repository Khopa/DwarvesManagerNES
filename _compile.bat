@rem compile C source file with given name into NES file
@rem useful to compile few projects at once without repeating the build script

@set CC65_HOME=..\cc65\

@if "%PATH%"=="%PATH:cc65=%" @PATH=%PATH%;%CC65_HOME%bin\

@ca65 crt0.s || goto fail

@cc65 -Oi DwarvesManager.c --add-source || goto fail

@ca65 DwarvesManager.s || goto fail

@ld65 -C nrom_128_horz.cfg -o DwarvesManager.nes crt0.o DwarvesManager.o runtime.lib || goto fail

@goto exit

:fail

pause

:exit

@del %1.s
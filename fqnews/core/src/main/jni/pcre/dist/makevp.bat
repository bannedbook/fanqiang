:: AH 20-12-06 modified for new PCRE-7.0 and VP/BCC
:: PH 19-03-07 renamed !compile.txt and !linklib.txt as makevp-compile.txt and
::             makevp-linklib.txt
:: PH 26-03-07 re-renamed !compile.txt and !linklib.txt as makevp-c.txt and
::             makevp-l.txt
:: PH 29-03-07 hopefully the final rename to makevp_c and makevp_l
:: AH 27.08.08 updated for new PCRE-7.7
::             required PCRE.H and CONFIG.H will be generated if not existing

@echo off
echo.
echo Compiling PCRE with BORLAND C++ for VIRTUAL PASCAL
echo.

REM This file was contributed by Alexander Tokarev for building PCRE for use
REM with Virtual Pascal. It has not been tested with the latest PCRE release.

REM This file has been modified and extended to compile with newer PCRE releases
REM by Stefan Weber (Angels Holocaust).

REM CHANGE THIS FOR YOUR BORLAND C++ COMPILER PATH
SET BORLAND=f:\bcc
REM location of the TASM binaries, if compiling with the -B BCC switch
SET TASM=f:\tasm

SET PATH=%PATH%;%BORLAND%\bin;%TASM%\bin
SET PCRE_VER=77
SET COMPILE_DEFAULTS=-DHAVE_CONFIG_H -DPCRE_STATIC -I%BORLAND%\include

del pcre%PCRE_VER%.lib >nul 2>nul

:: sh configure

:: check for needed header files
if not exist pcre.h copy pcre.h.generic pcre.h
if not exist config.h copy config.h.generic config.h

bcc32 -DDFTABLES %COMPILE_DEFAULTS% -L%BORLAND%\lib dftables.c
IF ERRORLEVEL 1 GOTO ERROR

:: dftables > chartables.c
dftables pcre_chartables.c

REM compile and link the PCRE library into lib: option -B for ASM compile works too
bcc32 -a4 -c -RT- -y- -v- -u- -R- -Q- -X -d -fp -ff -P- -O2 -Oc -Ov -3 -w-8004 -w-8064 -w-8065 -w-8012 -UDFTABLES -DVPCOMPAT %COMPILE_DEFAULTS% @makevp_c.txt
IF ERRORLEVEL 1 GOTO ERROR

tlib %BORLAND%\lib\cw32.lib *calloc *del *strncmp *memcpy *memmove *memset *memcmp *strlen
IF ERRORLEVEL 1 GOTO ERROR
tlib pcre%PCRE_VER%.lib @makevp_l.txt +calloc.obj +del.obj +strncmp.obj +memcpy.obj +memmove.obj +memset.obj +memcmp.obj +strlen.obj
IF ERRORLEVEL 1 GOTO ERROR

del *.obj *.tds *.bak >nul 2>nul

echo ---
echo Now the library should be complete. Please check all messages above.
echo Don't care for warnings, it's OK.
goto END

:ERROR
echo ---
echo Error while compiling PCRE. Aborting...
pause
goto END

:END

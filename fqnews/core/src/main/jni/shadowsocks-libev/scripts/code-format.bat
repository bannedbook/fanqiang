@echo off

set root=%~dp0
set source=%root%src

goto start

:format
set filelist=%1
for /r "%filelist%" %%f in (*) do (
  if "%%~xf" equ ".h" (
    call :format_file %%f
  ) else if "%%~xf" equ ".c" (
    call :format_file %%f
  )
)
goto end

:format_file
set f=%1
if "%~n1" neq "base64" (
  if "%~n1" neq "json" (
    if "%~n1" neq "uthash" (
      echo 'format file "%f%"'
      uncrustify -c %root%\.uncrustify.cfg -l C --replace --no-backup %f%
      DEL %~dp1*.uncrustify >nul 2>nul
    )
  )
)
goto end

:start
call :format %source%

:end

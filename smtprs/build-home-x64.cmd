@echo off
setlocal

set "ROOT=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
  echo vswhere.exe was not found.
  exit /b 1
)

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"

if not defined VSINSTALL (
  echo Visual Studio C++ build tools were not found.
  exit /b 1
)

call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

cd /d "%ROOT%"
if not exist build\obj mkdir build\obj
if not exist build\bin mkdir build\bin

set "COMMON_DEFS=/DWIN64 /DWIN32 /D_CONSOLE /D_MT /DWINDOWS /DMULTI_ML /DBTHREAD /DHOME_VERSION /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS"
set "COMMON_INC=/I. /I..\smtpds\compat /FIobjbase.h"
set "COMMON_CL=/nologo /c /W3 %COMMON_DEFS% %COMMON_INC%"

cl %COMMON_CL% /Fobuild\obj\auth.obj auth.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\aton.obj aton.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\bad.obj bad.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\base64.obj base64.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\ccopy.obj ccopy.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\Cluster.obj Cluster.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\data.obj data.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\dot.obj dot.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\effect.obj effect.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\ehlo.obj ehlo.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\expn.obj expn.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\filter.obj filter.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\header.obj header.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\helo.obj helo.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\help.obj help.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\ldap.obj ldap.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\mail.obj mail.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\md5c.obj md5c.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\milter.obj milter.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\mime_chk.obj mime_chk.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\noop.obj noop.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\ntoa.obj ntoa.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\ordb.obj ordb.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\pass.obj pass.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\profile.obj profile.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\QuickCV.obj QuickCV.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\quit.obj quit.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\Quoted.obj Quoted.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\rcpt.obj rcpt.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\Reg2File.obj Reg2File.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\regist.obj regist.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\rset.obj rset.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\service.obj service.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\smtprs.obj smtprs.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\socket.obj socket.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\spamx.obj spamx.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\stls.obj stls.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\Thirdparty.obj Thirdparty.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\threads.obj threads.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\user_manager.obj user_manager.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\userlist.obj userlist.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\vrfy.obj vrfy.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\utf7.obj utf7.cpp || exit /b 1
cl %COMMON_CL% /Fobuild\obj\utf8.obj utf8.cpp || exit /b 1

link /nologo /subsystem:console /machine:x64 /out:build\bin\epstrs-home-x64.exe ^
  build\obj\*.obj ^
  res\smtprs.RES ^
  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ^
  shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ^
  wsock32.lib netapi32.lib ws2_32.lib wldap32.lib dnsapi.lib
if errorlevel 1 exit /b 1

echo Built build\bin\epstrs-home-x64.exe

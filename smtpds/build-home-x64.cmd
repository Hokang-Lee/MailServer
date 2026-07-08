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

set "COMMON_DEFS=/DWIN64 /DWIN32 /D_CONSOLE /DWINNT /DUSE_OPTIONS_H /DUSE_UTIME /DWINDOWS /DNEW_SENDER /DMULTI_ML /DHOME_VERSION /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS"
set "COMMON_INC=/I. /Icompat /FIobjbase.h"
set "COMMON_CL=/nologo /c /W3 %COMMON_DEFS% %COMMON_INC%"

cl %COMMON_CL% /Fobuild\obj\base64.obj base64.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\ldap.obj ldap.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\lists.obj lists.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\mailctrl.obj mailctrl.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\md5c.obj md5c.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\mx_recde.obj mx_recde.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\profile.obj profile.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\Reg2File.obj Reg2File.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\regist.obj regist.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\sendmail.obj sendmail.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\service.obj service.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\smtpds.obj smtpds.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\user_manager.obj user_manager.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\userlist.obj userlist.c || exit /b 1
cl %COMMON_CL% /Fobuild\obj\legacy_msvc.obj compat\legacy_msvc.c || exit /b 1

link /nologo /subsystem:console /machine:x64 /out:build\bin\spads-home-x64.exe ^
  build\obj\base64.obj ^
  build\obj\ldap.obj ^
  build\obj\lists.obj ^
  build\obj\mailctrl.obj ^
  build\obj\md5c.obj ^
  build\obj\mx_recde.obj ^
  build\obj\profile.obj ^
  build\obj\Reg2File.obj ^
  build\obj\regist.obj ^
  build\obj\sendmail.obj ^
  build\obj\service.obj ^
  build\obj\smtpds.obj ^
  build\obj\user_manager.obj ^
  build\obj\userlist.obj ^
  build\obj\legacy_msvc.obj ^
  res\smtpds.RES ^
  ..\..\rel64\resolv.lib ^
  ..\..\rel64\wship6.lib ^
  kernel32.lib user32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ^
  odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib ^
  dnsapi.lib activeds.lib adsiid.lib
if errorlevel 1 exit /b 1

echo Built build\bin\spads-home-x64.exe

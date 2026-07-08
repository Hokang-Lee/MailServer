# Microsoft Developer Studio Project File - Name="smtpds" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=smtpds - Win32 Debug2
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "smtpds.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "smtpds.mak" CFG="smtpds - Win32 Debug2"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "smtpds - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtpds - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtpds - Win32 Release2" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtpds - Win32 Debug2" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtpds - Win32 Release3" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtpds - Win32 Debug3" ("Win32 (x86) Console Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "smtpds - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /GX /O2 /I "..\bind497\include" /I "..\bind497\compat\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ..\bind497\res\WinRel\resolv.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spads.exe" /MAPINFO:EXPORTS /MAPINFO:FIXUPS /MAPINFO:LINES
# SUBTRACT LINK32 /verbose /pdb:none /debug

!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bind497\include" /I "..\bind497\compat\include" /D "_DEBUG" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /FR /YX /FD /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\bind497\res\WinDebug\resolv.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spads.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "smtpds - Win32 Release2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "smtpds___Win32_Release2"
# PROP BASE Intermediate_Dir "smtpds___Win32_Release2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release2"
# PROP Intermediate_Dir "Release2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /GX /O2 /I "..\bind497\include" /I "..\bind497\compat\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /GX /O2 /I "..\bind497\include" /I "..\bind497\compat\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release2/epstds.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\bind497\res\WinRel\resolv.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spads.exe" /MAPINFO:EXPORTS /MAPINFO:FIXUPS /MAPINFO:LINES
# SUBTRACT BASE LINK32 /verbose /pdb:none /debug
# ADD LINK32 ..\bind497\res\WinRel\resolv.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.1.1a/Release-x86-MT/libcrypto.lib ../openssl-1.1.1a/Release-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release2/epstds.exe" /MAPINFO:EXPORTS /MAPINFO:FIXUPS /MAPINFO:LINES
# SUBTRACT LINK32 /verbose /pdb:none /debug

!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "smtpds___Win32_Debug2"
# PROP BASE Intermediate_Dir "smtpds___Win32_Debug2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug2"
# PROP Intermediate_Dir "Debug2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bind497\include" /I "..\bind497\compat\include" /D "_DEBUG" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bind497\include" /I "..\bind497\compat\include" /D "_DEBUG" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /FR /YX /FD /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug2/epstds.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\bind497\res\WinDebug\resolv.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spads.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\bind497\res\WinDebug\resolv.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.1.1a/Debug-x86-MT/libcrypto.lib ../openssl-1.1.1a/Debug-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug2/epstds.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "smtpds - Win32 Release3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "smtpds___Win32_Release3"
# PROP BASE Intermediate_Dir "smtpds___Win32_Release3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release3"
# PROP Intermediate_Dir "Release3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /GX /O2 /I "..\bind497\include" /I "..\bind497\compat\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /GX /O2 /I "..\bind497\include" /I "..\bind497\compat\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release3/epstds.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\bind497\res\WinRel\resolv.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spads.exe" /MAPINFO:EXPORTS /MAPINFO:FIXUPS /MAPINFO:LINES
# SUBTRACT BASE LINK32 /verbose /pdb:none /debug
# ADD LINK32 ..\bind497\res\WinRel\resolv.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-3.1.0/Release-x86-MT/libcrypto.lib ../openssl-3.1.0/Release-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release3/epstds.exe" /MAPINFO:EXPORTS /MAPINFO:FIXUPS /MAPINFO:LINES
# SUBTRACT LINK32 /verbose /pdb:none /debug

!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "smtpds___Win32_Debug3"
# PROP BASE Intermediate_Dir "smtpds___Win32_Debug3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug3"
# PROP Intermediate_Dir "Debug3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bind497\include" /I "..\bind497\compat\include" /D "_DEBUG" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bind497\include" /I "..\bind497\compat\include" /D "_DEBUG" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /FR /YX /FD /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug3/epstds.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\bind497\res\WinDebug\resolv.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spads.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\bind497\res\WinDebug\resolv.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-3.1.0/Debug-x86-MT/libcrypto.lib ../openssl-3.1.0/Debug-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug3/epstds.exe"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "smtpds - Win32 Release"
# Name "smtpds - Win32 Debug"
# Name "smtpds - Win32 Release2"
# Name "smtpds - Win32 Debug2"
# Name "smtpds - Win32 Release3"
# Name "smtpds - Win32 Debug3"
# Begin Source File

SOURCE=.\base64.c
# End Source File
# Begin Source File

SOURCE=.\ldap.c
# End Source File
# Begin Source File

SOURCE=.\lists.c
# End Source File
# Begin Source File

SOURCE=.\mailctrl.c
# End Source File
# Begin Source File

SOURCE=.\md5c.c
# End Source File
# Begin Source File

SOURCE=.\mx_recde.c
# End Source File
# Begin Source File

SOURCE=.\profile.c
# End Source File
# Begin Source File

SOURCE=.\Reg2File.c
# End Source File
# Begin Source File

SOURCE=.\regist.c
# End Source File
# Begin Source File

SOURCE=.\sendmail.c
# End Source File
# Begin Source File

SOURCE=.\service.c
# End Source File
# Begin Source File

SOURCE=.\service.h
# End Source File
# Begin Source File

SOURCE=.\smtp.h
# End Source File
# Begin Source File

SOURCE=.\smtpds.c
# End Source File
# Begin Source File

SOURCE=.\res\smtpds.h
# End Source File
# Begin Source File

SOURCE=.\res\smtpds.RES
# End Source File
# Begin Source File

SOURCE=.\user_manager.c
# End Source File
# Begin Source File

SOURCE=.\userlist.c
# End Source File
# End Target
# End Project

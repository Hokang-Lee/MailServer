# Microsoft Developer Studio Project File - Name="smtprs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=smtprs - Win32 Debug2
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "smtprs.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "smtprs.mak" CFG="smtprs - Win32 Debug2"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "smtprs - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtprs - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtprs - Win32 Release2" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtprs - Win32 Debug2" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtprs - Win32 Release3" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtprs - Win32 Debug3" ("Win32 (x86) Console Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "smtprs - Win32 Release"

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
# ADD CPP /nologo /MT /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spars.exe"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "smtprs - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "xSENDERID" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spars.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "smtprs - Win32 Release2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "smtprs___Win32_Release2"
# PROP BASE Intermediate_Dir "smtprs___Win32_Release2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release2"
# PROP Intermediate_Dir "Release2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spars.exe"
# SUBTRACT BASE LINK32 /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.1.1a/Release-x86-MT/libcrypto.lib ../openssl-1.1.1a/Release-x86-MT/libssl.lib wldap32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /machine:I386 /out:"Release2/epstrs.exe"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "smtprs - Win32 Debug2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "smtprs___Win32_Debug2"
# PROP BASE Intermediate_Dir "smtprs___Win32_Debug2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug2"
# PROP Intermediate_Dir "Debug2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "xSENDERID" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "xSENDERID" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spars.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.1.1a/Debug-x86-MT/libcrypto.lib ../openssl-1.1.1a/Debug-x86-MT/libssl.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug2/epstrs.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "smtprs - Win32 Release3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "smtprs___Win32_Release3"
# PROP BASE Intermediate_Dir "smtprs___Win32_Release3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release3"
# PROP Intermediate_Dir "Release3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-3.1.0/Release-x86-MT/libcrypto.lib ../openssl-3.1.0/Release-x86-MT/libssl.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/epstrs.exe"
# SUBTRACT BASE LINK32 /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-3.1.0/Release-x86-MT/libcrypto.lib ../openssl-3.1.0/Release-x86-MT/libssl.lib wldap32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /machine:I386 /out:"Release3/epstrs.exe"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "smtprs - Win32 Debug3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "smtprs___Win32_Debug3"
# PROP BASE Intermediate_Dir "smtprs___Win32_Debug3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug3"
# PROP Intermediate_Dir "Debug3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "xSENDERID" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "xSENDERID" /D "EIGHT_BITMIME" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libcrypto.lib ../openssl-1.0.2n/out32dll-Debug-MT/libssl.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spars.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-3.1.0/Debug-x86-MT/libcrypto.lib ../openssl-3.1.0/Debug-x86-MT/libssl.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug3/epstrs.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "smtprs - Win32 Release"
# Name "smtprs - Win32 Debug"
# Name "smtprs - Win32 Release2"
# Name "smtprs - Win32 Debug2"
# Name "smtprs - Win32 Release3"
# Name "smtprs - Win32 Debug3"
# Begin Source File

SOURCE=.\auth.c
# End Source File
# Begin Source File

SOURCE=.\bad.c
# End Source File
# Begin Source File

SOURCE=.\base64.c
# End Source File
# Begin Source File

SOURCE=.\ccopy.c
# End Source File
# Begin Source File

SOURCE=.\Cluster.c
# End Source File
# Begin Source File

SOURCE=.\data.c
# End Source File
# Begin Source File

SOURCE=.\dot.c
# End Source File
# Begin Source File

SOURCE=.\effect.c
# End Source File
# Begin Source File

SOURCE=.\ehlo.c
# End Source File
# Begin Source File

SOURCE=.\expn.c
# End Source File
# Begin Source File

SOURCE=.\filter.c
# End Source File
# Begin Source File

SOURCE=.\header.c
# End Source File
# Begin Source File

SOURCE=.\helo.c
# End Source File
# Begin Source File

SOURCE=.\help.c
# End Source File
# Begin Source File

SOURCE=.\ldap.c
# End Source File
# Begin Source File

SOURCE=.\mail.c
# End Source File
# Begin Source File

SOURCE=.\md5c.c
# End Source File
# Begin Source File

SOURCE=.\milter.c
# End Source File
# Begin Source File

SOURCE=.\mime_chk.c
# End Source File
# Begin Source File

SOURCE=.\noop.c
# End Source File
# Begin Source File

SOURCE=.\ordb.c
# End Source File
# Begin Source File

SOURCE=.\pass.c
# End Source File
# Begin Source File

SOURCE=.\profile.c
# End Source File
# Begin Source File

SOURCE=.\QuickCV.c
# End Source File
# Begin Source File

SOURCE=.\quit.c
# End Source File
# Begin Source File

SOURCE=.\Quoted.c
# End Source File
# Begin Source File

SOURCE=.\rcpt.c
# End Source File
# Begin Source File

SOURCE=.\Reg2File.c
# End Source File
# Begin Source File

SOURCE=.\regist.c
# End Source File
# Begin Source File

SOURCE=.\rset.c
# End Source File
# Begin Source File

SOURCE=.\service.c
# End Source File
# Begin Source File

SOURCE=.\smtp.h
# End Source File
# Begin Source File

SOURCE=.\smtprs.c
# End Source File
# Begin Source File

SOURCE=.\res\smtprs.h
# End Source File
# Begin Source File

SOURCE=.\res\smtprs.RES
# End Source File
# Begin Source File

SOURCE=.\socket.c
# End Source File
# Begin Source File

SOURCE=.\spamx.c
# End Source File
# Begin Source File

SOURCE=.\stls.c
# End Source File
# Begin Source File

SOURCE=.\Thirdparty.c
# End Source File
# Begin Source File

SOURCE=.\threads.c
# End Source File
# Begin Source File

SOURCE=.\user_manager.c
# End Source File
# Begin Source File

SOURCE=.\userlist.c
# End Source File
# Begin Source File

SOURCE=.\utf7.cpp
# End Source File
# Begin Source File

SOURCE=.\utf8.cpp
# End Source File
# Begin Source File

SOURCE=.\vrfy.c
# End Source File
# End Target
# End Project

# Microsoft Developer Studio Project File - Name="imap4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=imap4 - Win32 Debug2
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "imap4.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "imap4.mak" CFG="imap4 - Win32 Debug2"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "imap4 - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "imap4 - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE "imap4 - Win32 Release2" ("Win32 (x86) Console Application" 用)
!MESSAGE "imap4 - Win32 Debug2" ("Win32 (x86) Console Application" 用)
!MESSAGE "imap4 - Win32 Release3" ("Win32 (x86) Console Application" 用)
!MESSAGE "imap4 - Win32 Debug3" ("Win32 (x86) Console Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "imap4 - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release/spaimap4s.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spaimap4s.exe"

!ELSEIF  "$(CFG)" == "imap4 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug/spaimap4s.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spaimap4s.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "imap4 - Win32 Release2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "imap4___Win32_Release2"
# PROP BASE Intermediate_Dir "imap4___Win32_Release2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release2"
# PROP Intermediate_Dir "Release2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Release/spaimap4s.bsc"
# ADD BSC32 /nologo /o"Release2/epstimap4s.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spaimap4s.exe"
# ADD LINK32 ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.1.1a/Release-x86-MT/libcrypto.lib ../openssl-1.1.1a/Release-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /stack:0x200000 /subsystem:console /map /machine:I386 /out:"Release2/epstimap4s.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "imap4 - Win32 Debug2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "imap4___Win32_Debug2"
# PROP BASE Intermediate_Dir "imap4___Win32_Debug2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug2"
# PROP Intermediate_Dir "Debug2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Debug/spaimap4s.bsc"
# ADD BSC32 /nologo /o"Debug2/epstimap4s.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spaimap4s.exe" /pdbtype:sept
# ADD LINK32 ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.1.1a/Debug-x86-MT/libcrypto.lib ../openssl-1.1.1a/Debug-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /stack:0x200000 /subsystem:console /map /debug /machine:I386 /out:"Debug2/epstimap4s.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "imap4 - Win32 Release3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "imap4___Win32_Release3"
# PROP BASE Intermediate_Dir "imap4___Win32_Release3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release3"
# PROP Intermediate_Dir "Release3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Release/spaimap4s.bsc"
# ADD BSC32 /nologo /o"Release3/epstimap4s.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2n/out32dll-Release-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Release-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /machine:I386 /out:"Release/spaimap4s.exe"
# ADD LINK32 ..\ipv6-src\wship6\release\wship6.lib ../openssl-3.1.0/Release-x86-MT/libcrypto.lib ../openssl-3.1.0/Release-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /stack:0x200000 /subsystem:console /map /machine:I386 /out:"Release3/epstimap4s.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "imap4 - Win32 Debug3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "imap4___Win32_Debug3"
# PROP BASE Intermediate_Dir "imap4___Win32_Debug3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug3"
# PROP Intermediate_Dir "Debug3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Debug/spaimap4s.bsc"
# ADD BSC32 /nologo /o"Debug3/epstimap4s.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2n/out32dll-Debug-MT/libeay32.lib ../openssl-1.0.2n/out32dll-Debug-MT/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"Debug/spaimap4s.exe" /pdbtype:sept
# ADD LINK32 ..\ipv6-src\wship6\debug\wship6.lib ../openssl-3.1.0/Debug-x86-MT/libcrypto.lib ../openssl-3.1.0/Debug-x86-MT/libssl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /stack:0x200000 /subsystem:console /map /debug /machine:I386 /out:"Debug3/epstimap4s.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "imap4 - Win32 Release"
# Name "imap4 - Win32 Debug"
# Name "imap4 - Win32 Release2"
# Name "imap4 - Win32 Debug2"
# Name "imap4 - Win32 Release3"
# Name "imap4 - Win32 Debug3"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\append.c
# End Source File
# Begin Source File

SOURCE=.\authenticate.c
# End Source File
# Begin Source File

SOURCE=.\bad.c
# End Source File
# Begin Source File

SOURCE=.\base64.c
# End Source File
# Begin Source File

SOURCE=.\capability.c
# End Source File
# Begin Source File

SOURCE=.\check.c
# End Source File
# Begin Source File

SOURCE=.\close.c
# End Source File
# Begin Source File

SOURCE=.\copy.c
# End Source File
# Begin Source File

SOURCE=.\create.c
# End Source File
# Begin Source File

SOURCE=.\delete.c
# End Source File
# Begin Source File

SOURCE=.\examine.c
# End Source File
# Begin Source File

SOURCE=.\expunge.c
# End Source File
# Begin Source File

SOURCE=.\fetch.c
# End Source File
# Begin Source File

SOURCE=.\getmetadata.c
# End Source File
# Begin Source File

SOURCE=.\id.c
# End Source File
# Begin Source File

SOURCE=.\idle.c
# End Source File
# Begin Source File

SOURCE=.\imap4.c
# End Source File
# Begin Source File

SOURCE=.\imap4file.c
# End Source File
# Begin Source File

SOURCE=.\ldap.c
# End Source File
# Begin Source File

SOURCE=.\list.c
# End Source File
# Begin Source File

SOURCE=.\login.c
# End Source File
# Begin Source File

SOURCE=.\logout.c
# End Source File
# Begin Source File

SOURCE=.\lsub.c
# End Source File
# Begin Source File

SOURCE=.\md5c.c
# End Source File
# Begin Source File

SOURCE=.\namespace.c
# End Source File
# Begin Source File

SOURCE=.\noop.c
# End Source File
# Begin Source File

SOURCE=.\profile.c
# End Source File
# Begin Source File

SOURCE=.\recover.c
# End Source File
# Begin Source File

SOURCE=.\Reg2File.c
# End Source File
# Begin Source File

SOURCE=.\regist.c
# End Source File
# Begin Source File

SOURCE=.\rename.c
# End Source File
# Begin Source File

SOURCE=.\search.c
# End Source File
# Begin Source File

SOURCE=.\select.c
# End Source File
# Begin Source File

SOURCE=.\service.c
# End Source File
# Begin Source File

SOURCE=.\setmetadata.c
# End Source File
# Begin Source File

SOURCE=.\socket.c
# End Source File
# Begin Source File

SOURCE=.\status.c
# End Source File
# Begin Source File

SOURCE=.\stls.c
# End Source File
# Begin Source File

SOURCE=.\store.c
# End Source File
# Begin Source File

SOURCE=.\subscribe.c
# End Source File
# Begin Source File

SOURCE=.\threads.c
# End Source File
# Begin Source File

SOURCE=.\uid.c
# End Source File
# Begin Source File

SOURCE=.\unsubscribe.c
# End Source File
# Begin Source File

SOURCE=.\user_manager.c
# End Source File
# Begin Source File

SOURCE=.\userlist.c
# End Source File
# Begin Source File

SOURCE=.\utf8.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\codepage.h
# End Source File
# Begin Source File

SOURCE=.\res\imap4s.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\res\imap4s.RES
# End Source File
# End Target
# End Project

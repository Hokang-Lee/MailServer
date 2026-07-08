# Microsoft Developer Studio Generated NMAKE File, Based on smtprs.dsp
!IF "$(CFG)" == ""
CFG=smtprs - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの smtprs - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "smtprs - Win32 Release" && "$(CFG)" != "smtprs - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "smtprs.mak" CFG="smtprs - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "smtprs - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtprs - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "smtprs - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\spars.exe"


CLEAN :
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\bad.obj"
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\ccopy.obj"
	-@erase "$(INTDIR)\Cluster.obj"
	-@erase "$(INTDIR)\data.obj"
	-@erase "$(INTDIR)\dot.obj"
	-@erase "$(INTDIR)\effect.obj"
	-@erase "$(INTDIR)\ehlo.obj"
	-@erase "$(INTDIR)\expn.obj"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\header.obj"
	-@erase "$(INTDIR)\helo.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\ldap.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\milter.obj"
	-@erase "$(INTDIR)\mime_chk.obj"
	-@erase "$(INTDIR)\noop.obj"
	-@erase "$(INTDIR)\ordb.obj"
	-@erase "$(INTDIR)\pass.obj"
	-@erase "$(INTDIR)\profile.obj"
	-@erase "$(INTDIR)\QuickCV.obj"
	-@erase "$(INTDIR)\quit.obj"
	-@erase "$(INTDIR)\Quoted.obj"
	-@erase "$(INTDIR)\rcpt.obj"
	-@erase "$(INTDIR)\Reg2File.obj"
	-@erase "$(INTDIR)\regist.obj"
	-@erase "$(INTDIR)\rset.obj"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\smtprs.obj"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\spamx.obj"
	-@erase "$(INTDIR)\stls.obj"
	-@erase "$(INTDIR)\Thirdparty.obj"
	-@erase "$(INTDIR)\threads.obj"
	-@erase "$(INTDIR)\user_manager.obj"
	-@erase "$(INTDIR)\userlist.obj"
	-@erase "$(INTDIR)\utf7.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vrfy.obj"
	-@erase "$(OUTDIR)\spars.exe"
	-@erase "$(OUTDIR)\spars.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "EIGHT_BITMIME" /D "VC6" /Fp"$(INTDIR)\smtprs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\smtprs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2e/out32x86dll-Release/libeay32.lib ../openssl-1.0.2e/out32x86dll-Release/ssleay32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\spars.pdb" /map:"$(INTDIR)\spars.map" /machine:I386 /out:"$(OUTDIR)\spars.exe" 
LINK32_OBJS= \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\bad.obj" \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\ccopy.obj" \
	"$(INTDIR)\Cluster.obj" \
	"$(INTDIR)\data.obj" \
	"$(INTDIR)\dot.obj" \
	"$(INTDIR)\effect.obj" \
	"$(INTDIR)\ehlo.obj" \
	"$(INTDIR)\expn.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\header.obj" \
	"$(INTDIR)\helo.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\ldap.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\milter.obj" \
	"$(INTDIR)\mime_chk.obj" \
	"$(INTDIR)\noop.obj" \
	"$(INTDIR)\ordb.obj" \
	"$(INTDIR)\pass.obj" \
	"$(INTDIR)\profile.obj" \
	"$(INTDIR)\QuickCV.obj" \
	"$(INTDIR)\quit.obj" \
	"$(INTDIR)\Quoted.obj" \
	"$(INTDIR)\rcpt.obj" \
	"$(INTDIR)\Reg2File.obj" \
	"$(INTDIR)\regist.obj" \
	"$(INTDIR)\rset.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\smtprs.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\spamx.obj" \
	"$(INTDIR)\stls.obj" \
	"$(INTDIR)\Thirdparty.obj" \
	"$(INTDIR)\threads.obj" \
	"$(INTDIR)\user_manager.obj" \
	"$(INTDIR)\userlist.obj" \
	"$(INTDIR)\utf7.obj" \
	"$(INTDIR)\utf8.obj" \
	"$(INTDIR)\vrfy.obj" \
	".\res\smtprs.RES"

"$(OUTDIR)\spars.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "smtprs - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\spars.exe"


CLEAN :
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\bad.obj"
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\ccopy.obj"
	-@erase "$(INTDIR)\Cluster.obj"
	-@erase "$(INTDIR)\data.obj"
	-@erase "$(INTDIR)\dot.obj"
	-@erase "$(INTDIR)\effect.obj"
	-@erase "$(INTDIR)\ehlo.obj"
	-@erase "$(INTDIR)\expn.obj"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\header.obj"
	-@erase "$(INTDIR)\helo.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\ldap.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\milter.obj"
	-@erase "$(INTDIR)\mime_chk.obj"
	-@erase "$(INTDIR)\noop.obj"
	-@erase "$(INTDIR)\ordb.obj"
	-@erase "$(INTDIR)\pass.obj"
	-@erase "$(INTDIR)\profile.obj"
	-@erase "$(INTDIR)\QuickCV.obj"
	-@erase "$(INTDIR)\quit.obj"
	-@erase "$(INTDIR)\Quoted.obj"
	-@erase "$(INTDIR)\rcpt.obj"
	-@erase "$(INTDIR)\Reg2File.obj"
	-@erase "$(INTDIR)\regist.obj"
	-@erase "$(INTDIR)\rset.obj"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\smtprs.obj"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\spamx.obj"
	-@erase "$(INTDIR)\stls.obj"
	-@erase "$(INTDIR)\Thirdparty.obj"
	-@erase "$(INTDIR)\threads.obj"
	-@erase "$(INTDIR)\user_manager.obj"
	-@erase "$(INTDIR)\userlist.obj"
	-@erase "$(INTDIR)\utf7.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vrfy.obj"
	-@erase "$(OUTDIR)\spars.exe"
	-@erase "$(OUTDIR)\spars.ilk"
	-@erase "$(OUTDIR)\spars.map"
	-@erase "$(OUTDIR)\spars.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "xSENDERID" /D "EIGHT_BITMIME" /D "VC6" /Fp"$(INTDIR)\smtprs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\smtprs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2e/out32x86dll-Debug/libeay32.lib ../openssl-1.0.2e/out32x86dll-Debug/ssleay32.lib wldap32.lib Dnsapi.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\spars.pdb" /map:"$(INTDIR)\spars.map" /debug /machine:I386 /out:"$(OUTDIR)\spars.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\bad.obj" \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\ccopy.obj" \
	"$(INTDIR)\Cluster.obj" \
	"$(INTDIR)\data.obj" \
	"$(INTDIR)\dot.obj" \
	"$(INTDIR)\effect.obj" \
	"$(INTDIR)\ehlo.obj" \
	"$(INTDIR)\expn.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\header.obj" \
	"$(INTDIR)\helo.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\ldap.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\milter.obj" \
	"$(INTDIR)\mime_chk.obj" \
	"$(INTDIR)\noop.obj" \
	"$(INTDIR)\ordb.obj" \
	"$(INTDIR)\pass.obj" \
	"$(INTDIR)\profile.obj" \
	"$(INTDIR)\QuickCV.obj" \
	"$(INTDIR)\quit.obj" \
	"$(INTDIR)\Quoted.obj" \
	"$(INTDIR)\rcpt.obj" \
	"$(INTDIR)\Reg2File.obj" \
	"$(INTDIR)\regist.obj" \
	"$(INTDIR)\rset.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\smtprs.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\spamx.obj" \
	"$(INTDIR)\stls.obj" \
	"$(INTDIR)\Thirdparty.obj" \
	"$(INTDIR)\threads.obj" \
	"$(INTDIR)\user_manager.obj" \
	"$(INTDIR)\userlist.obj" \
	"$(INTDIR)\utf7.obj" \
	"$(INTDIR)\utf8.obj" \
	"$(INTDIR)\vrfy.obj" \
	".\res\smtprs.RES"

"$(OUTDIR)\spars.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("smtprs.dep")
!INCLUDE "smtprs.dep"
!ELSE 
!MESSAGE Warning: cannot find "smtprs.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "smtprs - Win32 Release" || "$(CFG)" == "smtprs - Win32 Debug"
SOURCE=.\auth.c

"$(INTDIR)\auth.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\bad.c

"$(INTDIR)\bad.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\base64.c

"$(INTDIR)\base64.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ccopy.c

"$(INTDIR)\ccopy.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Cluster.c

"$(INTDIR)\Cluster.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\data.c

"$(INTDIR)\data.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dot.c

"$(INTDIR)\dot.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\effect.c

"$(INTDIR)\effect.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ehlo.c

"$(INTDIR)\ehlo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\expn.c

"$(INTDIR)\expn.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\filter.c

"$(INTDIR)\filter.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\header.c

"$(INTDIR)\header.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\helo.c

"$(INTDIR)\helo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\help.c

"$(INTDIR)\help.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ldap.c

"$(INTDIR)\ldap.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mail.c

"$(INTDIR)\mail.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\md5c.c

"$(INTDIR)\md5c.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\milter.c

"$(INTDIR)\milter.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mime_chk.c

"$(INTDIR)\mime_chk.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\noop.c

"$(INTDIR)\noop.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ordb.c

"$(INTDIR)\ordb.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\pass.c

"$(INTDIR)\pass.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\profile.c

"$(INTDIR)\profile.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\QuickCV.c

"$(INTDIR)\QuickCV.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\quit.c

"$(INTDIR)\quit.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Quoted.c

"$(INTDIR)\Quoted.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rcpt.c

"$(INTDIR)\rcpt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Reg2File.c

"$(INTDIR)\Reg2File.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\regist.c

"$(INTDIR)\regist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rset.c

"$(INTDIR)\rset.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\service.c

"$(INTDIR)\service.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\smtprs.c

"$(INTDIR)\smtprs.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\socket.c

"$(INTDIR)\socket.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\spamx.c

"$(INTDIR)\spamx.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\stls.c

"$(INTDIR)\stls.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Thirdparty.c

"$(INTDIR)\Thirdparty.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\threads.c

"$(INTDIR)\threads.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\user_manager.c

"$(INTDIR)\user_manager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\userlist.c

"$(INTDIR)\userlist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utf7.cpp

"$(INTDIR)\utf7.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utf8.cpp

"$(INTDIR)\utf8.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vrfy.c

"$(INTDIR)\vrfy.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 


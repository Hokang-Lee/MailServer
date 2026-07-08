# Microsoft Developer Studio Generated NMAKE File, Based on imap4.dsp
!IF "$(CFG)" == ""
CFG=imap4 - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの imap4 - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "imap4 - Win32 Release" && "$(CFG)" != "imap4 - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "imap4.mak" CFG="imap4 - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "imap4 - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "imap4 - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "imap4 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\spaimap4s.exe"


CLEAN :
	-@erase "$(INTDIR)\append.obj"
	-@erase "$(INTDIR)\authenticate.obj"
	-@erase "$(INTDIR)\bad.obj"
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\capability.obj"
	-@erase "$(INTDIR)\check.obj"
	-@erase "$(INTDIR)\close.obj"
	-@erase "$(INTDIR)\copy.obj"
	-@erase "$(INTDIR)\create.obj"
	-@erase "$(INTDIR)\delete.obj"
	-@erase "$(INTDIR)\examine.obj"
	-@erase "$(INTDIR)\expunge.obj"
	-@erase "$(INTDIR)\fetch.obj"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\imap4.obj"
	-@erase "$(INTDIR)\imap4file.obj"
	-@erase "$(INTDIR)\ldap.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\login.obj"
	-@erase "$(INTDIR)\logout.obj"
	-@erase "$(INTDIR)\lsub.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\namespace.obj"
	-@erase "$(INTDIR)\noop.obj"
	-@erase "$(INTDIR)\profile.obj"
	-@erase "$(INTDIR)\recover.obj"
	-@erase "$(INTDIR)\Reg2File.obj"
	-@erase "$(INTDIR)\regist.obj"
	-@erase "$(INTDIR)\rename.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\select.obj"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\status.obj"
	-@erase "$(INTDIR)\stls.obj"
	-@erase "$(INTDIR)\store.obj"
	-@erase "$(INTDIR)\subscribe.obj"
	-@erase "$(INTDIR)\threads.obj"
	-@erase "$(INTDIR)\uid.obj"
	-@erase "$(INTDIR)\unsubscribe.obj"
	-@erase "$(INTDIR)\user_manager.obj"
	-@erase "$(INTDIR)\userlist.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\spaimap4s.exe"
	-@erase "$(OUTDIR)\spaimap4s.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /Fp"$(INTDIR)\imap4.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\spaimap4s.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2e/out32x86dll-Release/libeay32.lib ../openssl-1.0.2e/out32x86dll-Release/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\spaimap4s.pdb" /map:"$(INTDIR)\spaimap4s.map" /machine:I386 /out:"$(OUTDIR)\spaimap4s.exe" 
LINK32_OBJS= \
	"$(INTDIR)\append.obj" \
	"$(INTDIR)\authenticate.obj" \
	"$(INTDIR)\bad.obj" \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\capability.obj" \
	"$(INTDIR)\check.obj" \
	"$(INTDIR)\close.obj" \
	"$(INTDIR)\copy.obj" \
	"$(INTDIR)\create.obj" \
	"$(INTDIR)\delete.obj" \
	"$(INTDIR)\examine.obj" \
	"$(INTDIR)\expunge.obj" \
	"$(INTDIR)\fetch.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\imap4.obj" \
	"$(INTDIR)\imap4file.obj" \
	"$(INTDIR)\ldap.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\login.obj" \
	"$(INTDIR)\logout.obj" \
	"$(INTDIR)\lsub.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\namespace.obj" \
	"$(INTDIR)\noop.obj" \
	"$(INTDIR)\profile.obj" \
	"$(INTDIR)\recover.obj" \
	"$(INTDIR)\Reg2File.obj" \
	"$(INTDIR)\regist.obj" \
	"$(INTDIR)\rename.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\select.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\status.obj" \
	"$(INTDIR)\stls.obj" \
	"$(INTDIR)\store.obj" \
	"$(INTDIR)\subscribe.obj" \
	"$(INTDIR)\threads.obj" \
	"$(INTDIR)\uid.obj" \
	"$(INTDIR)\unsubscribe.obj" \
	"$(INTDIR)\user_manager.obj" \
	"$(INTDIR)\userlist.obj" \
	"$(INTDIR)\utf8.obj" \
	".\res\imap4s.RES"

"$(OUTDIR)\spaimap4s.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "imap4 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\spaimap4s.exe"


CLEAN :
	-@erase "$(INTDIR)\append.obj"
	-@erase "$(INTDIR)\authenticate.obj"
	-@erase "$(INTDIR)\bad.obj"
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\capability.obj"
	-@erase "$(INTDIR)\check.obj"
	-@erase "$(INTDIR)\close.obj"
	-@erase "$(INTDIR)\copy.obj"
	-@erase "$(INTDIR)\create.obj"
	-@erase "$(INTDIR)\delete.obj"
	-@erase "$(INTDIR)\examine.obj"
	-@erase "$(INTDIR)\expunge.obj"
	-@erase "$(INTDIR)\fetch.obj"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\imap4.obj"
	-@erase "$(INTDIR)\imap4file.obj"
	-@erase "$(INTDIR)\ldap.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\login.obj"
	-@erase "$(INTDIR)\logout.obj"
	-@erase "$(INTDIR)\lsub.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\namespace.obj"
	-@erase "$(INTDIR)\noop.obj"
	-@erase "$(INTDIR)\profile.obj"
	-@erase "$(INTDIR)\recover.obj"
	-@erase "$(INTDIR)\Reg2File.obj"
	-@erase "$(INTDIR)\regist.obj"
	-@erase "$(INTDIR)\rename.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\select.obj"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\status.obj"
	-@erase "$(INTDIR)\stls.obj"
	-@erase "$(INTDIR)\store.obj"
	-@erase "$(INTDIR)\subscribe.obj"
	-@erase "$(INTDIR)\threads.obj"
	-@erase "$(INTDIR)\uid.obj"
	-@erase "$(INTDIR)\unsubscribe.obj"
	-@erase "$(INTDIR)\user_manager.obj"
	-@erase "$(INTDIR)\userlist.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\spaimap4s.exe"
	-@erase "$(OUTDIR)\spaimap4s.ilk"
	-@erase "$(OUTDIR)\spaimap4s.map"
	-@erase "$(OUTDIR)\spaimap4s.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /D "WINDOWS" /D "MULTI_ML" /D "BTHREAD" /D "USE_SSL" /D "VC6" /Fp"$(INTDIR)\imap4.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\spaimap4s.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2e/out32x86dll-Debug/libeay32.lib ../openssl-1.0.2e/out32x86dll-Debug/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\spaimap4s.pdb" /map:"$(INTDIR)\spaimap4s.map" /debug /machine:I386 /out:"$(OUTDIR)\spaimap4s.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\append.obj" \
	"$(INTDIR)\authenticate.obj" \
	"$(INTDIR)\bad.obj" \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\capability.obj" \
	"$(INTDIR)\check.obj" \
	"$(INTDIR)\close.obj" \
	"$(INTDIR)\copy.obj" \
	"$(INTDIR)\create.obj" \
	"$(INTDIR)\delete.obj" \
	"$(INTDIR)\examine.obj" \
	"$(INTDIR)\expunge.obj" \
	"$(INTDIR)\fetch.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\imap4.obj" \
	"$(INTDIR)\imap4file.obj" \
	"$(INTDIR)\ldap.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\login.obj" \
	"$(INTDIR)\logout.obj" \
	"$(INTDIR)\lsub.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\namespace.obj" \
	"$(INTDIR)\noop.obj" \
	"$(INTDIR)\profile.obj" \
	"$(INTDIR)\recover.obj" \
	"$(INTDIR)\Reg2File.obj" \
	"$(INTDIR)\regist.obj" \
	"$(INTDIR)\rename.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\select.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\status.obj" \
	"$(INTDIR)\stls.obj" \
	"$(INTDIR)\store.obj" \
	"$(INTDIR)\subscribe.obj" \
	"$(INTDIR)\threads.obj" \
	"$(INTDIR)\uid.obj" \
	"$(INTDIR)\unsubscribe.obj" \
	"$(INTDIR)\user_manager.obj" \
	"$(INTDIR)\userlist.obj" \
	"$(INTDIR)\utf8.obj" \
	".\res\imap4s.RES"

"$(OUTDIR)\spaimap4s.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("imap4.dep")
!INCLUDE "imap4.dep"
!ELSE 
!MESSAGE Warning: cannot find "imap4.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "imap4 - Win32 Release" || "$(CFG)" == "imap4 - Win32 Debug"
SOURCE=.\append.c

"$(INTDIR)\append.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\authenticate.c

"$(INTDIR)\authenticate.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\bad.c

"$(INTDIR)\bad.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\base64.c

"$(INTDIR)\base64.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\capability.c

"$(INTDIR)\capability.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\check.c

"$(INTDIR)\check.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\close.c

"$(INTDIR)\close.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\copy.c

"$(INTDIR)\copy.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\create.c

"$(INTDIR)\create.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\delete.c

"$(INTDIR)\delete.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\examine.c

"$(INTDIR)\examine.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\expunge.c

"$(INTDIR)\expunge.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fetch.c

"$(INTDIR)\fetch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\idle.c

"$(INTDIR)\idle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\imap4.c

"$(INTDIR)\imap4.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\imap4file.c

"$(INTDIR)\imap4file.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ldap.c

"$(INTDIR)\ldap.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\list.c

"$(INTDIR)\list.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\login.c

"$(INTDIR)\login.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\logout.c

"$(INTDIR)\logout.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\lsub.c

"$(INTDIR)\lsub.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\md5c.c

"$(INTDIR)\md5c.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\namespace.c

"$(INTDIR)\namespace.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\noop.c

"$(INTDIR)\noop.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\profile.c

"$(INTDIR)\profile.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\recover.c

"$(INTDIR)\recover.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Reg2File.c

"$(INTDIR)\Reg2File.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\regist.c

"$(INTDIR)\regist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rename.c

"$(INTDIR)\rename.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\search.c

"$(INTDIR)\search.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\select.c

"$(INTDIR)\select.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\service.c

"$(INTDIR)\service.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\socket.c

"$(INTDIR)\socket.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\status.c

"$(INTDIR)\status.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\stls.c

"$(INTDIR)\stls.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\store.c

"$(INTDIR)\store.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\subscribe.c

"$(INTDIR)\subscribe.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\threads.c

"$(INTDIR)\threads.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\uid.c

"$(INTDIR)\uid.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\unsubscribe.c

"$(INTDIR)\unsubscribe.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\user_manager.c

"$(INTDIR)\user_manager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\userlist.c

"$(INTDIR)\userlist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utf8.cpp

"$(INTDIR)\utf8.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 


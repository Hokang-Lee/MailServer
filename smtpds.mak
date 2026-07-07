# Microsoft Developer Studio Generated NMAKE File, Based on smtpds.dsp
!IF "$(CFG)" == ""
CFG=smtpds - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの smtpds - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "smtpds - Win32 Release" && "$(CFG)" != "smtpds - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "smtpds.mak" CFG="smtpds - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "smtpds - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "smtpds - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "smtpds - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\spads.exe"


CLEAN :
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\ldap.obj"
	-@erase "$(INTDIR)\lists.obj"
	-@erase "$(INTDIR)\mailctrl.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\mx_recde.obj"
	-@erase "$(INTDIR)\profile.obj"
	-@erase "$(INTDIR)\Reg2File.obj"
	-@erase "$(INTDIR)\regist.obj"
	-@erase "$(INTDIR)\sendmail.obj"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\smtpds.obj"
	-@erase "$(INTDIR)\user_manager.obj"
	-@erase "$(INTDIR)\userlist.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\spads.exe"
	-@erase "$(OUTDIR)\spads.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /GX /O2 /I "..\bind497\include" /I "..\bind497\compat\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /Fp"$(INTDIR)\smtpds.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\smtpds.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\bind497\res\WinRel\resolv.lib ..\ipv6-src\wship6\release\wship6.lib ../openssl-1.0.2e/out32x86dll-Release/libeay32.lib ../openssl-1.0.2e/out32x86dll-Release/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\spads.pdb" /map:"$(INTDIR)\spads.map" /machine:I386 /out:"$(OUTDIR)\spads.exe" /MAPINFO:EXPORTS /MAPINFO:FIXUPS /MAPINFO:LINES 
LINK32_OBJS= \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\ldap.obj" \
	"$(INTDIR)\lists.obj" \
	"$(INTDIR)\mailctrl.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\mx_recde.obj" \
	"$(INTDIR)\profile.obj" \
	"$(INTDIR)\Reg2File.obj" \
	"$(INTDIR)\regist.obj" \
	"$(INTDIR)\sendmail.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\smtpds.obj" \
	"$(INTDIR)\user_manager.obj" \
	"$(INTDIR)\userlist.obj" \
	".\res\smtpds.RES"

"$(OUTDIR)\spads.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\spads.exe" "$(OUTDIR)\smtpds.bsc"


CLEAN :
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\base64.sbr"
	-@erase "$(INTDIR)\ldap.obj"
	-@erase "$(INTDIR)\ldap.sbr"
	-@erase "$(INTDIR)\lists.obj"
	-@erase "$(INTDIR)\lists.sbr"
	-@erase "$(INTDIR)\mailctrl.obj"
	-@erase "$(INTDIR)\mailctrl.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\mx_recde.obj"
	-@erase "$(INTDIR)\mx_recde.sbr"
	-@erase "$(INTDIR)\profile.obj"
	-@erase "$(INTDIR)\profile.sbr"
	-@erase "$(INTDIR)\Reg2File.obj"
	-@erase "$(INTDIR)\Reg2File.sbr"
	-@erase "$(INTDIR)\regist.obj"
	-@erase "$(INTDIR)\regist.sbr"
	-@erase "$(INTDIR)\sendmail.obj"
	-@erase "$(INTDIR)\sendmail.sbr"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\service.sbr"
	-@erase "$(INTDIR)\smtpds.obj"
	-@erase "$(INTDIR)\smtpds.sbr"
	-@erase "$(INTDIR)\user_manager.obj"
	-@erase "$(INTDIR)\user_manager.sbr"
	-@erase "$(INTDIR)\userlist.obj"
	-@erase "$(INTDIR)\userlist.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\smtpds.bsc"
	-@erase "$(OUTDIR)\spads.exe"
	-@erase "$(OUTDIR)\spads.ilk"
	-@erase "$(OUTDIR)\spads.map"
	-@erase "$(OUTDIR)\spads.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bind497\include" /I "..\bind497\compat\include" /D "_DEBUG" /D "WINNT" /D "USE_OPTIONS_H" /D "i386" /D "USE_UTIME" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WINDOWS" /D "NEW_SENDER" /D "MULTI_ML" /D "USE_SSL" /D "VC6" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\smtpds.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\smtpds.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\base64.sbr" \
	"$(INTDIR)\ldap.sbr" \
	"$(INTDIR)\lists.sbr" \
	"$(INTDIR)\mailctrl.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\mx_recde.sbr" \
	"$(INTDIR)\profile.sbr" \
	"$(INTDIR)\Reg2File.sbr" \
	"$(INTDIR)\regist.sbr" \
	"$(INTDIR)\sendmail.sbr" \
	"$(INTDIR)\service.sbr" \
	"$(INTDIR)\smtpds.sbr" \
	"$(INTDIR)\user_manager.sbr" \
	"$(INTDIR)\userlist.sbr"

"$(OUTDIR)\smtpds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\bind497\res\WinDebug\resolv.lib ..\ipv6-src\wship6\debug\wship6.lib ../openssl-1.0.2e/out32x86dll-Debug/libeay32.lib ../openssl-1.0.2e/out32x86dll-Debug/ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib netapi32.lib ws2_32.lib wldap32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\spads.pdb" /map:"$(INTDIR)\spads.map" /debug /machine:I386 /out:"$(OUTDIR)\spads.exe" 
LINK32_OBJS= \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\ldap.obj" \
	"$(INTDIR)\lists.obj" \
	"$(INTDIR)\mailctrl.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\mx_recde.obj" \
	"$(INTDIR)\profile.obj" \
	"$(INTDIR)\Reg2File.obj" \
	"$(INTDIR)\regist.obj" \
	"$(INTDIR)\sendmail.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\smtpds.obj" \
	"$(INTDIR)\user_manager.obj" \
	"$(INTDIR)\userlist.obj" \
	".\res\smtpds.RES"

"$(OUTDIR)\spads.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("smtpds.dep")
!INCLUDE "smtpds.dep"
!ELSE 
!MESSAGE Warning: cannot find "smtpds.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "smtpds - Win32 Release" || "$(CFG)" == "smtpds - Win32 Debug"
SOURCE=.\base64.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\base64.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\base64.obj"	"$(INTDIR)\base64.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\ldap.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\ldap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\ldap.obj"	"$(INTDIR)\ldap.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\lists.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\lists.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\lists.obj"	"$(INTDIR)\lists.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mailctrl.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\mailctrl.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\mailctrl.obj"	"$(INTDIR)\mailctrl.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\md5c.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\md5c.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\md5c.obj"	"$(INTDIR)\md5c.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mx_recde.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\mx_recde.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\mx_recde.obj"	"$(INTDIR)\mx_recde.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\profile.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\profile.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\profile.obj"	"$(INTDIR)\profile.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Reg2File.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\Reg2File.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\Reg2File.obj"	"$(INTDIR)\Reg2File.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\regist.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\regist.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\regist.obj"	"$(INTDIR)\regist.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\sendmail.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\sendmail.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\sendmail.obj"	"$(INTDIR)\sendmail.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\service.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\service.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\service.obj"	"$(INTDIR)\service.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\smtpds.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\smtpds.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\smtpds.obj"	"$(INTDIR)\smtpds.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\user_manager.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\user_manager.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\user_manager.obj"	"$(INTDIR)\user_manager.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\userlist.c

!IF  "$(CFG)" == "smtpds - Win32 Release"


"$(INTDIR)\userlist.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smtpds - Win32 Debug"


"$(INTDIR)\userlist.obj"	"$(INTDIR)\userlist.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 


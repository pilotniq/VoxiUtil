# Microsoft Developer Studio Project File - Name="VoxiUtil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=VoxiUtil - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VoxiUtil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VoxiUtil.mak" CFG="VoxiUtil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VoxiUtil - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VoxiUtil - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VoxiUtil - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VOXIUTIL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VOXIUTIL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "VoxiUtil - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VOXIUTIL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\Documents and Settings\erland\My Documents\Development\voxi\util\util\include" /I "recognizer\include" /I "C:\Documents and Settings\erland\My Documents\Development\pthreads" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VOXIUTIL_EXPORTS" /D "LIB_UTIL_INTERNAL" /D "PTHREADS_WIN32" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib pthreadVCE.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\Pre-built\lib" /libpath:"C:\Documents and Settings\erland\My Documents\Development\voxi\util\Debug"

!ENDIF 

# Begin Target

# Name "VoxiUtil - Win32 Release"
# Name "VoxiUtil - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\util\src\bag.c
# End Source File
# Begin Source File

SOURCE=.\util\src\bitFippling.c
# End Source File
# Begin Source File

SOURCE=.\util\src\bt.c
# End Source File
# Begin Source File

SOURCE=..\..\IcepeakUtil\byteQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\util\src\circularBuffer.c
# End Source File
# Begin Source File

SOURCE=.\util\src\driver.c
# End Source File
# Begin Source File

SOURCE=.\util\src\err.c
# End Source File
# Begin Source File

SOURCE=.\util\src\file.c
# End Source File
# Begin Source File

SOURCE=.\util\src\geometry.c
# End Source File
# Begin Source File

SOURCE=.\util\src\hash.c
# End Source File
# Begin Source File

SOURCE=.\util\src\idTable.c
# End Source File
# Begin Source File

SOURCE=.\util\src\libcCompat.c
# End Source File
# Begin Source File

SOURCE=.\util\src\mem.c
# End Source File
# Begin Source File

SOURCE=.\util\src\memory.c
# End Source File
# Begin Source File

SOURCE=.\util\src\path.c
# End Source File
# Begin Source File

SOURCE=.\util\src\shlib.c
# End Source File
# Begin Source File

SOURCE=.\util\src\sock.c
# End Source File
# Begin Source File

SOURCE=.\util\src\stateMachine.c
# End Source File
# Begin Source File

SOURCE=.\util\src\strbuf.c
# End Source File
# Begin Source File

SOURCE=.\util\src\textRPC.c
# End Source File
# Begin Source File

SOURCE=.\util\src\threading.c
# End Source File
# Begin Source File

SOURCE=.\util\src\threadpool.c
# End Source File
# Begin Source File

SOURCE=.\util\src\time.c
# End Source File
# Begin Source File

SOURCE=.\util\src\vector.c
# End Source File
# Begin Source File

SOURCE=.\util\src\win32_glue.c
# End Source File
# Begin Source File

SOURCE=.\util\src\wordMap.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "voxi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\util\include\voxi\alwaysInclude.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\util\include\voxi\util\bag.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\bitFippling.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\bt.h
# End Source File
# Begin Source File

SOURCE=..\..\IcepeakUtil\byteQueue.hpp
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\circularBuffer.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\collection.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\config.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\cvsid.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\debug.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\driver.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\err.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\file.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\geometry.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\hash.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\idTable.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\libcCompat.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\license.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\mem.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\memory.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\path.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\shlib.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\sock.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\strbuf.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\textRPC.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\threading.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\threadpool.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\time.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\types.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\vector.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\win32_glue.h
# End Source File
# Begin Source File

SOURCE=.\util\include\voxi\util\wordMap.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
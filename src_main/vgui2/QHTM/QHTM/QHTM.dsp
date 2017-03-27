# Microsoft Developer Studio Project File - Name="QHTM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=QHTM - Win32 Debug Light
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "QHTM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "QHTM.mak" CFG="QHTM - Win32 Debug Light"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "QHTM - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "QHTM - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "QHTM - Win32 Debug Light" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "QHTM - Win32 Release Light" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Components/QHTM", FIHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "QHTM - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "QHTM_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W4 /WX /GX /O2 /I "../" /I "..\\" /D "QHTM_DLL" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\ImgLib.lib ..\ZLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../QHTM.pdb" /machine:I386 /def:".\QHTM.def" /out:"..\QHTM.dll" /implib:"../QHTM.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "QHTM - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "QHTM_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /WX /Gm /GX /ZI /Od /I "..\\" /D "QHTM_DLL" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\ImgLibD.lib ..\ZLibD.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../QHTMd.pdb" /debug /machine:I386 /def:".\QHTM.def" /out:"..\QHTMd.dll" /implib:"../QHTMd.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "QHTM - Win32 Debug Light"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "QHTM___Win32_Debug_Light"
# PROP BASE Intermediate_Dir "QHTM___Win32_Debug_Light"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "QHTM___Win32_Debug_Light"
# PROP Intermediate_Dir "QHTM___Win32_Debug_Light"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W4 /WX /Gm /GX /ZI /Od /D "QHTM_DLL" /D "ALLOW_PRINT" /D "COOLTIPS" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /WX /Gm /GX /ZI /Od /I "../" /I "..\\" /D "QHTM_BUILD_LIGHT" /D "QHTM_DLL" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ImgLibD.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../QHTMd.pdb" /debug /machine:I386 /def:".\QHTM.def" /out:"..\QHTMd.dll" /implib:"../QHTMd.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\ImgLibD.lib ..\ZLibD.lib debughlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../QHTMLightd.pdb" /debug /machine:I386 /def:".\QHTM.def" /out:"..\QHTMLightd.dll" /implib:"../QHTMLightd.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "QHTM - Win32 Release Light"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "QHTM___Win32_Release_Light"
# PROP BASE Intermediate_Dir "QHTM___Win32_Release_Light"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "QHTM___Win32_Release_Light"
# PROP Intermediate_Dir "QHTM___Win32_Release_Light"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W4 /WX /GX /O2 /D "QHTM_DLL" /D "ALLOW_PRINT" /D "COOLTIPS" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x400 /D "SHOWSPLASH" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W4 /WX /GX /O2 /I "../" /I "..\\" /D "QHTM_BUILD_LIGHT" /D "QHTM_DLL" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ImgLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../QHTM.pdb" /machine:I386 /def:".\QHTM.def" /out:"..\QHTM.dll" /implib:"../QHTM.lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\ImgLibLight.lib ..\ZLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"../QHTMLight.pdb" /machine:I386 /def:".\QHTM.def" /out:"..\QHTMLight.dll" /implib:"../QHTMLight.lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "QHTM - Win32 Release"
# Name "QHTM - Win32 Debug"
# Name "QHTM - Win32 Debug Light"
# Name "QHTM - Win32 Release Light"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Shared Stuff"

# PROP Default_Filter ""
# Begin Group "Section"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\HTMLHorizontalRuleSection.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLHorizontalRuleSection.h
# End Source File
# Begin Source File

SOURCE=.\HTMLImageSection.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLImageSection.h
# End Source File
# Begin Source File

SOURCE=.\HTMLSection.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLSection.h
# End Source File
# Begin Source File

SOURCE=.\HTMLSectionABC.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLSectionABC.h
# End Source File
# Begin Source File

SOURCE=.\HTMLSectionCreator.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLSectionCreator.h
# End Source File
# Begin Source File

SOURCE=.\HTMLSectionLink.cpp
# End Source File
# Begin Source File

SOURCE=.\HtmlSectionLink.h
# End Source File
# Begin Source File

SOURCE=.\htmltablesection.cpp
# End Source File
# Begin Source File

SOURCE=.\htmltablesection.h
# End Source File
# Begin Source File

SOURCE=.\HTMLTextSection.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLTextSection.h
# End Source File
# Begin Source File

SOURCE=.\ParentSection.cpp
# End Source File
# Begin Source File

SOURCE=.\ParentSection.h
# End Source File
# Begin Source File

SOURCE=.\ScrollContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollContainer.h
# End Source File
# Begin Source File

SOURCE=.\sectionabc.cpp
# End Source File
# Begin Source File

SOURCE=.\sectionabc.h
# End Source File
# Begin Source File

SOURCE=.\TipWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\TipWindow.h
# End Source File
# Begin Source File

SOURCE=.\WindowSection.cpp
# End Source File
# Begin Source File

SOURCE=.\WindowSection.h
# End Source File
# End Group
# Begin Group "HTML"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DecodeCharset.cpp
# End Source File
# Begin Source File

SOURCE=.\GetFontSize.cpp
# End Source File
# Begin Source File

SOURCE=.\HtmlAnchor.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLAnchor.h
# End Source File
# Begin Source File

SOURCE=.\htmlblockquote.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLBlockQuote.h
# End Source File
# Begin Source File

SOURCE=.\HTMLDocument.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLDocument.h
# End Source File
# Begin Source File

SOURCE=.\HTMLDocumentObject.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLDocumentObject.h
# End Source File
# Begin Source File

SOURCE=.\HTMLGetAlignmentFromString.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLHorizontalRule.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLHorizontalRule.h
# End Source File
# Begin Source File

SOURCE=.\HTMLImage.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLImage.h
# End Source File
# Begin Source File

SOURCE=.\HTMLLink.h
# End Source File
# Begin Source File

SOURCE=.\HtmlList.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLList.h
# End Source File
# Begin Source File

SOURCE=.\HTMLListItem.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLListItem.h
# End Source File
# Begin Source File

SOURCE=.\HTMLParagraph.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLParagraph.h
# End Source File
# Begin Source File

SOURCE=.\HTMLParagraphObject.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLParagraphObject.h
# End Source File
# Begin Source File

SOURCE=.\HTMLParse.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLParse.h
# End Source File
# Begin Source File

SOURCE=.\HTMLTable.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLTable.h
# End Source File
# Begin Source File

SOURCE=.\HTMLTableCell.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLTableCell.h
# End Source File
# Begin Source File

SOURCE=.\HTMLTextBlock.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLTextBlock.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\cursor.h
# End Source File
# Begin Source File

SOURCE=.\drawcontext.cpp
# End Source File
# Begin Source File

SOURCE=.\drawcontext.h
# End Source File
# Begin Source File

SOURCE=.\GetColourFromString.cpp
# End Source File
# Begin Source File

SOURCE=.\GetFontSizeAsPixels.cpp
# End Source File
# Begin Source File

SOURCE=.\GetNumberParameter.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLFontDef.h
# End Source File
# Begin Source File

SOURCE=.\HTMLPrint.cpp
# End Source File
# Begin Source File

SOURCE=.\Mouse.cpp
# End Source File
# Begin Source File

SOURCE=.\SimpleString.cpp
# End Source File
# Begin Source File

SOURCE=.\SimpleString.h
# End Source File
# Begin Source File

SOURCE=.\smallstringhash.h
# End Source File
# Begin Source File

SOURCE=.\tablelayout.cpp
# End Source File
# Begin Source File

SOURCE=.\tablelayout.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# End Group
# Begin Group "Reuse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GetDisplayWorkArea.cpp
# End Source File
# Begin Source File

SOURCE=.\GotoUrl.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLParseBase.cpp
# End Source File
# Begin Source File

SOURCE=.\ResourceLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\ResourceLoader.h
# End Source File
# Begin Source File

SOURCE=.\striremove.cpp
# End Source File
# Begin Source File

SOURCE=.\stristr.cpp
# End Source File
# Begin Source File

SOURCE=.\strremove.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\AquireImage.cpp
# End Source File
# Begin Source File

SOURCE=.\AquireImage.h
# End Source File
# Begin Source File

SOURCE=.\Defaults.cpp
# End Source File
# Begin Source File

SOURCE=.\Defaults.h
# End Source File
# Begin Source File

SOURCE=.\dllmain.cpp
# End Source File
# Begin Source File

SOURCE=.\EnableCoolTips.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLParseBase.h
# End Source File
# Begin Source File

SOURCE=.\LoadTextFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Palette.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM.def

!IF  "$(CFG)" == "QHTM - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "QHTM - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "QHTM - Win32 Debug Light"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "QHTM - Win32 Release Light"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QHTM.h
# End Source File
# Begin Source File

SOURCE=.\QHTM.rc
# End Source File
# Begin Source File

SOURCE=.\QHTM_GetHTMLHeight.cpp
# End Source File
# Begin Source File

SOURCE=.\qhtm_includes.h
# End Source File
# Begin Source File

SOURCE=.\QHTM_MessageBox.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM_RenderHTML.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM_SetHTMLButton.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM_SetResources.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM_Trace.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTM_Trace.h
# End Source File
# Begin Source File

SOURCE=.\QHTM_Types.h
# End Source File
# Begin Source File

SOURCE=.\QHTMControlSection.cpp
# End Source File
# Begin Source File

SOURCE=.\QHTMControlSection.h
# End Source File
# Begin Source File

SOURCE=.\RegisterWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\StaticString.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Array.h
# End Source File
# Begin Source File

SOURCE=..\Array.inl
# End Source File
# Begin Source File

SOURCE=..\DebugHlp.h
# End Source File
# Begin Source File

SOURCE=..\Map.h
# End Source File
# Begin Source File

SOURCE=..\Map.inl
# End Source File
# Begin Source File

SOURCE=..\MapIter.h
# End Source File
# Begin Source File

SOURCE=..\MapIter.inl
# End Source File
# Begin Source File

SOURCE=.\Palette.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\reuse\ResourceLoader.h
# End Source File
# Begin Source File

SOURCE=..\Stack.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\reuse\Store.h
# End Source File
# Begin Source File

SOURCE=..\reuse\Stream.h
# End Source File
# Begin Source File

SOURCE=..\TextABC.h
# End Source File
# Begin Source File

SOURCE=..\WindowText.h
# End Source File
# Begin Source File

SOURCE=..\WinHelper.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\arrow.cur
# End Source File
# Begin Source File

SOURCE=.\res\hand1.cur
# End Source File
# Begin Source File

SOURCE=.\res\noimage.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\History.txt
# End Source File
# Begin Source File

SOURCE=".\How to Build.txt"
# End Source File
# Begin Source File

SOURCE=.\Licence.txt
# End Source File
# Begin Source File

SOURCE=".\Stuff to do.txt"
# End Source File
# End Target
# End Project

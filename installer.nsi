!include "MUI2.nsh"

Name "Sudoku Demo"
OutFile "Sudoku_demo_Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\Sudoku Demo"
RequestExecutionLevel user

!define MUI_ABORTWARNING

; Pages
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Launch application checkbox using custom function to support paths with spaces
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchApp"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

Section "Install"
  SetOutPath "$INSTDIR"
  
  ; Copy files recursively from release_pkg
  File /r "release_pkg\*"

  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\Sudoku Demo"
  CreateShortcut "$SMPROGRAMS\Sudoku Demo\Sudoku Demo.lnk" "$INSTDIR\Sudoku_demo.exe"
  CreateShortcut "$DESKTOP\Sudoku Demo.lnk" "$INSTDIR\Sudoku_demo.exe"

  ; Write uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Add to Add/Remove Programs (User Level)
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sudoku Demo" "DisplayName" "Sudoku Demo"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sudoku Demo" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sudoku Demo" "DisplayIcon" "$INSTDIR\Sudoku_demo.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sudoku Demo" "Publisher" "Daedalus"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\Sudoku Demo.lnk"
  Delete "$SMPROGRAMS\Sudoku Demo\Sudoku Demo.lnk"
  RMDir /r "$SMPROGRAMS\Sudoku Demo"

  ; Remove uninstaller first, then remove all remaining files and the installation directory
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR"

  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sudoku Demo"
SectionEnd

Function LaunchApp
  ExecShell "" "$INSTDIR\Sudoku_demo.exe"
FunctionEnd

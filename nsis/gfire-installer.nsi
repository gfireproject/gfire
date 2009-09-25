; Script based on pidgin-facebook installer

SetCompressor /SOLID lzma

; HM NIS Edit Wizard helper defines
;!define GFIRE_VERSION "0.9.0-svn"
;!define GFIRE_INSTALL_DIR "./win32-install"
!define PRODUCT_NAME "Gfire"
!define PRODUCT_VERSION ${GFIRE_VERSION}
!define PRODUCT_PUBLISHER "Gfire Team"
!define PRODUCT_WEB_SITE "http://gfireproject.org"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!define GFIRE_DIR ".."

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON			".\gfx\gfire.ico"
!define MUI_UNICON			".\gfx\gfire.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP	".\gfx\gfire-intro.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP		".\gfx\gfire-header.bmp"


; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE 		"${GFIRE_DIR}\COPYING"
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT		"Run Pidgin"
!define MUI_FINISHPAGE_RUN_FUNCTION	"RunPidgin"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
;!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE 		"English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\pidgin-gfire-${PRODUCT_VERSION}.exe"

Var "PidginDir"

ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
    ;Check for pidgin installation
    Call GetPidginInstPath

    SetOverwrite try
	uninst:
		ClearErrors
		Delete "$PidginDir\plugins\libxfire.dll"
		IfErrors dllbusy
		Goto after_uninst
	dllbusy:
		MessageBox MB_RETRYCANCEL "libxfire.dll is busy. Please close Pidgin (including tray icon) and try again." IDCANCEL Cancel
		Goto uninst
	cancel:
		Abort "Installation of Gfire aborted"
	after_uninst:

	SetOutPath $PidginDir
	File /r ${GFIRE_DIR}/${GFIRE_INSTALL_DIR}/*.*
		
SectionEnd

Function GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	ReadRegStr $0 HKCU "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
		MessageBox MB_OK|MB_ICONINFORMATION "Failed to find Pidgin installation."
		Abort "Failed to find Pidgin installation. Please install Pidgin first."
  cont:
	StrCpy $PidginDir $0
FunctionEnd

Function RunPidgin
	ExecShell "" "$PidginDir\pidgin.exe"
FunctionEnd


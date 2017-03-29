Name "VirtualWebcam"
OutFile "VirtualWebcam.exe"
InstallDir $PROGRAMFILES\VirtualWebcam
InstallDirRegKey HKLM "Software\NSIS_VirtualWebcam" "Install_Dir"
RequestExecutionLevel admin

Page components 
Page directory 
Page instfiles 

UninstPage uninstConfirm
UninstPage instfiles

Section "VirtualWebcam (required)"

	MessageBox MB_OK "Please close all video chat applications like Skype before installing."
	SectionIn RO
	
	SetOutPath $INSTDIR
	File "VCam.ax"
	File "*.dll"
	File "*.xml"

	RegDLL $INSTDIR\VCam.ax
	Sleep 1000
	
	IfErrors 0 NoError
		Delete "$PROGRAMFILES\VirtualWebcam\*";
		;Remove directories used
		RMDir "$PROGRAMFILES\VirtualWebcam"
		RMDir "$INSTDIR"
		MessageBox MB_OK "Installation failed. Make sure you have admin rights."
		Goto RegError
	
	NoError:
		;write installation path in reg
		WriteRegStr HKLM Software\NSIS_VirtualWebcam "Install_Dir" "$INSTDIR"
		
		;write uninstallation path
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VirtualWebcam" "DisplayName" "NSIS VirtualWebcam"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VirtualWebcam" "UninstallString" '"$INSTDIR\VirtualWebcam_uninstall.exe"'
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VirtualWebcam" "NoModify" 1
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VirtualWebcam" "NoRepair" 1
		WriteUninstaller "VirtualWebcam_uninstall.exe"
	
	RegError:

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

	IfErrors RegError 0
	  CreateDirectory "$SMPROGRAMS\VirtualWebcam"
	  CreateShortCut "$SMPROGRAMS\VirtualWebcam\VirtualWebcam_Uninstall.lnk" "$INSTDIR\VirtualWebcam_uninstall.exe" "" "$INSTDIR\VirtualWebcam_uninstall.exe" 0
	  ;CreateShortCut "$SMPROGRAMS\VirtualWebcam\VirtualWebcam.lnk" "$INSTDIR\VirtualWebcamCode.exe" "" "$INSTDIR\VirtualWebcamCode.exe" 0
  
  RegError:
  
SectionEnd

Section "Uninstall"
  
  MessageBox MB_OK "Please close all video chat applications like Skype before uninstalling."
  
  ; Remove registry keys
  UnRegDLL $INSTDIR\VCam.ax
  Sleep 1000
  
  IfErrors 0 NoError
	MessageBox MB_OK "Uinstallation failed. Please make sure no video chat application is using this program."
	Goto UnRegError

  NoError:
	  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VirtualWebcam"
	  DeleteRegKey HKLM SOFTWARE\NSIS_VirtualWebcam

	  ; Remove files and uninstaller
	  Delete $INSTDIR\VirtualWebcam.nsi
	  Delete $INSTDIR\uninstall.exe

	  ; Remove shortcuts, if any
	  Delete "$SMPROGRAMS\VirtualWebcam\*.*"
	  Delete "$PROGRAMFILES\VirtualWebcam\*";

	  ; Remove directories used
	  RMDir "$SMPROGRAMS\VirtualWebcam"
	  RMDir "$PROGRAMFILES\VirtualWebcam"
	  RMDir "$INSTDIR"
  
  UnRegError:
  

SectionEnd
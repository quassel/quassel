; basic script template for NullsoftInstallerPackager
;
; Copyright 2013 Patrick von Reth <vonreth@kde.org>
; Copyright 2010 Patrick Spendrin <ps_ml@gmx.de>
; adapted from marble.nsi

var ToBeRunned
var nameOfToBeRunend

!define productname "Quassel"
!define company "KDE"

!include MUI2.nsh
!include LogicLib.nsh
!include SnoreNotify.nsh


; registry stuff
!define regkey "Software\${company}\${productname}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\Quassel"

!define startmenu "$SMPROGRAMS\${productname}"
!define uninstaller "uninstall.exe"

Var StartMenuFolder

!define PRODUCT_WEB_SITE https://quassel-irc.org/
!define MyApp_AppUserModelId  QuasselProject.QuasselIRC
!define SnoreToastExe "$INSTDIR\SnoreToast.exe"

;Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${regkey}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

InstType "Full"
;--------------------------------


XPStyle on
ShowInstDetails hide
ShowUninstDetails hide

SetCompressor /SOLID lzma

Name ${productname}
Caption "${caption}"

OutFile "${setupname}"

!define MUI_ICON ${gitDir}\pics\quassel.ico

!insertmacro MUI_PAGE_WELCOME

;!insertmacro MUI_PAGE_LICENSE
#${license}
;!insertmacro MUI_PAGE_LICENSE

!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN $ToBeRunned
!define MUI_FINISHPAGE_RUN_TEXT $nameOfToBeRunend
!define MUI_FINISHPAGE_LINK "Visit project homepage"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"
!insertmacro MUI_PAGE_FINISH

;uninstaller
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
;-------

!insertmacro MUI_LANGUAGE "English"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

InstallDir "${defaultinstdir}\${productname}"
InstallDirRegKey HKLM "${regkey}" ""


;--------------------------------
AutoCloseWindow false


; beginning (invisible) section
Section "--hidden Quassel Base" QUASSEL_BASE
   SectionIn RO
   SetOutPath $INSTDIR
   SetShellVarContext all
   StrCpy $ToBeRunned ""

    WriteRegStr HKLM "${regkey}" "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM "${regkey}" "Version" "${version}"
    WriteRegStr HKLM "${regkey}" "" "$INSTDIR\uninstall.exe"

    WriteRegStr HKLM "${uninstkey}" "DisplayName" "Quassel (remove only)"
    WriteRegStr HKLM "${uninstkey}" "DisplayIcon" "$INSTDIR\${MUI_ICON}"
    WriteRegStr HKLM "${uninstkey}" "DisplayVersion" "${version}"
    WriteRegStr HKLM "${uninstkey}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'
    WriteRegStr HKLM "${uninstkey}" "Publisher" "${company}"

  SetOutPath $INSTDIR


    ; package all files, recursively, preserving attributes
    ; assume files are in the correct places

    File /a /r /x "*.nsi" /x "*quassel.exe" /x "*quasselclient.exe" /x "*quasselcore.exe" /x "${setupname}" "${srcdir}\*.*"
    File /a  ${MUI_ICON}

    !if "${vcredist}" != "none"
        File /a /oname=vcredist.exe "${vcredist}"
        ExecWait '"$INSTDIR\vcredist.exe" /passive'
        Delete "$INSTDIR\vcredist.exe"
    !endif

    WriteUninstaller "${uninstaller}"


    ;Create shortcuts
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd


Section "Quassel"  QUASSEL_ALL_IN_ONE
    SectionIn 1
    SetOutPath $INSTDIR
    StrCpy $ToBeRunned "$INSTDIR\quassel.exe"
    StrCpy $nameOfToBeRunend "Run Quassel"
    File /a /oname=quassel.exe "${srcdir}\quassel.exe"
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        !insertmacro SnoreShortcut "$SMPROGRAMS\$StartMenuFolder\Quassel.lnk" "$INSTDIR\quassel.exe" "${MyApp_AppUserModelId}"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "QuasselClient"  QUASSEL_CLIENT
    SectionIn 1
    SetOutPath $INSTDIR
    ${If} $ToBeRunned == ""
        StrCpy $ToBeRunned "$INSTDIR\quasselclient.exe"
        StrCpy $nameOfToBeRunend "Run QuasselClient"
    ${Endif}
    File /a /oname=quasselclient.exe "${srcdir}\quasselclient.exe"
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        !insertmacro SnoreShortcut "$SMPROGRAMS\$StartMenuFolder\Quassel Client.lnk" "$INSTDIR\quasselclient.exe" "${MyApp_AppUserModelId}"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "QuasselCore"  QUASSEL_CORE
    SectionIn 1
    SetOutPath $INSTDIR
    ${If} $ToBeRunned == ""
        StrCpy $ToBeRunned "$INSTDIR\quasselcore.exe"
        StrCpy $nameOfToBeRunend "Run QuasselCore"
    ${Endif}
     File /a /oname=quasselcore.exe "${srcdir}\quasselcore.exe"
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Quassel Core.lnk" "$INSTDIR\quasselcore.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

; Section /o "QuasselCoreService"  QUASSEL_CORE_SERVICE
    ; SimpleSC::ExistsService "QuasselCore"
    ; Pop $0 ; returns an errorcode if the service doesn't exists (<>0)/service exists (0)
    ; ${If} $0 == 0
        ; MessageBox MB_OK|MB_ICONSTOP "Install Service QUassel failed - Reason: Service already exists"
    ; ${Else}
        ; SimpleSC::InstallService "QuasselCore" "QuasselCore" "16" "2" "$INSTDIR\bin\quasselcore.exe" "" "" ""
        ; Pop $0 ; returns an errorcode (<>0) otherwise success (0)
            ; ${If} $0 != 0
                ; Push $0
                ; SimpleSC::GetErrorMessage
                ; Pop $0
                ; MessageBox MB_OK|MB_ICONSTOP "Install of Service QUassel failed - Reason: $0"
            ; ${Else}
                ; SimpleSC::StartService "QuasselCore" "" 30
                ; Pop $0 ; returns an errorcode (<>0) otherwise success (0)
                ; ${If} $0 != 0
                    ; Push $0
                    ; SimpleSC::GetErrorMessage
                    ; Pop $0
                    ; MessageBox MB_OK|MB_ICONSTOP "Install of Service QUassel failed - Reason: $0"
                ; ${EndIf}
            ; ${EndIf}
    ; ${EndIf}
; SectionEnd

; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller

UninstallText "This will uninstall Quassel."

Section "Uninstall"
    SetShellVarContext all
    SetShellVarContext all

    DeleteRegKey HKLM "${uninstkey}"
    DeleteRegKey HKLM "${regkey}"

    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder


  ; SimpleSC::ExistsService "QuasselCore"
  ; Pop $0   ; returns an errorcode if the service doesn't exists (<>0)/service exists (0)
  ; ${If} $0 == 0
    ; SimpleSC::ServiceIsStopped "QuasselCore"
    ; Pop $0 ; returns an errorcode (<>0) otherwise success (0)
    ; Pop $1 ; returns 1 (service is stopped) - returns 0 (service is not stopped)
    ; ${If} $0 == 0
    ; ${AndIf} $1 == 0
        ; SimpleSC::StopService "QuasselCore" 1 30
        ; Pop $0 ; returns an errorcode (<>0) otherwise success (0)
        ; ${If} $0 != 0
            ; Push $0
            ; SimpleSC::GetErrorMessage
            ; Pop $0
            ; MessageBox MB_OK|MB_ICONSTOP "Stopping failed - Reason: $0"
        ; ${Else}
             ; SimpleSC::RemoveService "QuasselCore"
             ; Pop $0 ; returns an errorcode (<>0) otherwise success (0)
             ; ${If} $0 != 0
                    ; Push $0
                    ; SimpleSC::GetErrorMessage
                    ; Pop $0
                    ; MessageBox MB_OK|MB_ICONSTOP "Remove fails - Reason: $0"
                ; ${EndIf}
        ; ${EndIf}
    ; ${EndIf}
  ; ${EndIf}

    RMDir /r "$SMPROGRAMS\$StartMenuFolder"
    RMDir /r "$INSTDIR"
SectionEnd

Function .onSelChange
    ${If} ${SectionIsSelected} ${QUASSEL_CORE}
    ${OrIf}  ${SectionIsSelected} ${QUASSEL_CLIENT}
    ${OrIf}  ${SectionIsSelected} ${QUASSEL_ALL_IN_ONE}
        GetDlgItem $0 $HWNDPARENT 1
        EnableWindow $0 1
    ${Else}
        GetDlgItem $0 $HWNDPARENT 1
        EnableWindow $0 0
    ${EndIf}
FunctionEnd

Function .onInit
    ReadRegStr $R0 HKLM "${uninstkey}" "UninstallString"
    StrCmp $R0 "" done
    ReadRegStr $INSTDIR HKLM "${regkey}" "Install_Dir"
    ;Run the uninstaller
    ;uninst:
    ClearErrors
    ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
    done:
FunctionEnd

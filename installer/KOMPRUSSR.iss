#define PluginName "KOMPRUSSR KU-01"
#ifndef PluginVersion
  #define PluginVersion "1.0.0"
#endif

[Setup]
AppId={{7B3E2A14-5C9D-4F6A-8E21-3D0A9B4C7F58}
AppName={#PluginName}
AppVersion={#PluginVersion}
AppPublisher=pedepena
; Carpeta VST3 estandar de Windows. No se pregunta nada: solo instalar.
DefaultDirName={commoncf64}\VST3
UsePreviousAppDir=no
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=yes
DisableReadyPage=yes
Uninstallable=yes
UninstallDisplayName={#PluginName}
OutputDir=output
OutputBaseFilename=KOMPRUSSR-Setup
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "..\build\KOMPRUSSR_artefacts\Release\VST3\KOMPRUSSR KU-01.vst3\*"; DestDir: "{app}\KOMPRUSSR KU-01.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{app}\KOMPRUSSR KU-01.vst3"

[Code]
function InitializeSetup: Boolean;
var
  Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  if (not IsWin64) or (Version.Major <> 10) or (Version.ProductType <> VER_NT_WORKSTATION) then
  begin
    SuppressibleMsgBox('KOMPRUSSR KU-01 requiere Windows 10 u 11 de 64 bits. No se ha instalado nada.',
      mbCriticalError, MB_OK, IDOK);
    Result := False;
    Exit;
  end;
  Result := True;
end;

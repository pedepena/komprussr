#define PluginName "KOMPRUSSR KU-01"
#ifndef PluginVersion
  #define PluginVersion "0.1.0"
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
ArchitecturesAllowed=x64
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

; KOMPRUSSR KU-01 Windows installer (Inno Setup 6).
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef Vst3Dir
  #error Missing /DVst3Dir=... (path to the compiled VST3 bundle)
#endif
#ifndef RepoDir
  #define RepoDir "."
#endif

[Setup]
AppId={{30DE2563-6255-4E5A-9AB2-4BBB6EBE0597}
AppName=KOMPRUSSR KU-01
AppVersion={#AppVersion}
AppPublisher=KOMPRUSSR
AppPublisherURL=https://github.com/pedepena/komprussr
CreateAppDir=no
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
Uninstallable=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#RepoDir}\dist
OutputBaseFilename=KOMPRUSSR-KU-01-{#AppVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
LicenseFile={#RepoDir}\LICENSE
WizardStyle=modern

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
spanish.WelcomeLabel2=Este asistente instalará KOMPRUSSR KU-01 VST3 en la carpeta común de plugins de Windows.
english.WelcomeLabel2=This wizard will install KOMPRUSSR KU-01 VST3 in the common Windows plug-in folder.
spanish.FinishedLabel=La instalación ha terminado. Abre tu programa de música y vuelve a escanear los plugins.
english.FinishedLabel=Installation is complete. Open your music software and rescan its plug-ins.

[Files]
Source: "{#Vst3Dir}\*"; DestDir: "{commoncf64}\VST3\Komprussr KU-01.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Komprussr KU-01.vst3"

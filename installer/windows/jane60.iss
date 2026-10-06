; Jane-Sixty Windows installer (Inno Setup 6). CI passes /DVersion=x.y.z /DSourceDir=<dir with Jane-Sixty.vst3 and Jane-Sixty.exe>.
#ifndef Version
  #define Version "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "dist-src"
#endif

[Setup]
AppId={{7B1E2C9A-5D3F-4B7A-9C2E-6A1F0D4E8B21}
AppName=Jane-Sixty
AppVersion={#Version}
AppPublisher=clevergear
AppPublisherURL=https://github.com/pirassic/jnsynth
DefaultDirName={autopf}\clevergear\Jane-Sixty
DefaultGroupName=Jane-Sixty
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputBaseFilename=Jane-Sixty-{#Version}-win64-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
UninstallDisplayIcon={app}\Jane-Sixty.exe

[Components]
Name: "vst3"; Description: "VST3 plugin (Common Files\VST3)"; Types: full compact custom; Flags: fixed
Name: "app"; Description: "Standalone application"; Types: full custom

[Files]
Source: "{#SourceDir}\Jane-Sixty.vst3\*"; DestDir: "{commoncf64}\VST3\Jane-Sixty.vst3"; Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#SourceDir}\Jane-Sixty.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{group}\Jane-Sixty"; Filename: "{app}\Jane-Sixty.exe"; Components: app
Name: "{group}\Uninstall Jane-Sixty"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\Jane-Sixty.exe"; Description: "Launch Jane-Sixty"; Flags: postinstall nowait skipifsilent; Components: app

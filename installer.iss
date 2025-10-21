; --------------------------------------
; Plugin Installer Script for MRS-R
; --------------------------------------

#define PluginName "MRS-R"
#define CompanyName "StairwayAudio"
#define Version "1.0"
#define BuildDir "build"

[Setup]
AppName={#PluginName}
AppVersion={#Version}
DefaultDirName={pf}\{#CompanyName}\{#PluginName}
DefaultGroupName={#CompanyName}
OutputBaseFilename={#PluginName}Installer   
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64 x86
DisableProgramGroupPage=yes

[Files]
; VST3 plugin goes to the common VST3 folder. Install 
Source: "{#BuildDir}\{#PluginName}_artefacts\Release\VST3\{#PluginName}.vst3"; DestDir: "{commoncf}\VST3"; \
    Flags: recursesubdirs ignoreversion createallsubdirs

[Icons]
Name: "{group}\Uninstall {#PluginName}"; Filename: "{uninstallexe}"

[UninstallDelete]
; Optionally clean up extra folders or settings
; Type: dir|file; Name: "full\path"
; Example: Type: dir; Name: "{app}\Presets"

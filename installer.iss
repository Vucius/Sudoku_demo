[Setup]
AppName=Sudoku Demo
AppVersion=1.0
AppPublisher=Daedalus
AppPublisherURL=https://github.com/
AppSupportURL=https://github.com/
AppUpdatesURL=https://github.com/
DefaultDirName={autopf}\Sudoku Demo
DefaultGroupName=Sudoku Demo
OutputDir=.
OutputBaseFilename=Sudoku_demo_Setup
Compression=lzma2
SolidCompression=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\Sudoku_demo.exe

[Files]
Source: "release_pkg\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Sudoku Demo"; Filename: "{app}\Sudoku_demo.exe"
Name: "{autodesktop}\Sudoku Demo"; Filename: "{app}\Sudoku_demo.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Run]
Filename: "{app}\Sudoku_demo.exe"; Description: "Launch Sudoku Demo"; Flags: nowait postinstall skipifsilent

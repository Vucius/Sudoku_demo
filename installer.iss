[Setup]
AppName=Sudoku Demo
AppVersion=1.0
DefaultDirName={autopf}\Sudoku Demo
DefaultGroupName=Sudoku Demo
OutputDir=.
OutputBaseFilename=Sudoku_demo_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
DisableProgramGroupPage=yes
PrivilegesRequired=lowest

[Files]
Source: "release_pkg\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Sudoku Demo"; Filename: "{app}\Sudoku_demo.exe"
Name: "{autodesktop}\Sudoku Demo"; Filename: "{app}\Sudoku_demo.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

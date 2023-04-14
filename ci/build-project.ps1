param(
    [string]$ProjectDir = ".",
    [string]$Name = "Release_x64",
    [string]$Arch = "x64",
    [string]$Configuration = "Release",
    [string]$BuildMethod = "cmake"
)

./c/build-project.ps1 -RepoName "common-cxx" -ProjectDir $ProjectDir -Name $Name -Configuration $Configuration -BuildMethod $BuildMethod

exit $LASTEXITCODE
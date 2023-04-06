param(
    [string]$ProjectDir = ".",
    [string]$Name = "Release_x64",
    [string]$Arch = "x64",
    [string]$Configuration = "Release"
)

./c/run-integration-tests.ps1 -RepoName "common-cxx-test" -ProjectDir $ProjectDir -Name $Name -Configuration $Configuration -Arch $Arch

exit $LASTEXITCODE

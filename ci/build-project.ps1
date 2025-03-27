param(
    [string]$RepoName,
    [string]$ProjectDir = ".",
    [string]$Name = "Release_x64",
    [string]$Arch = "x64",
    [string]$Configuration = "Release",
    [string]$BuildMethod = "cmake"
)

# This is common logic, so let's call the common script
./cxx/build-project.ps1 -RepoName $RepoName -ProjectDir $ProjectDir -Name $Name -Configuration $Configuration -BuildMethod $BuildMethod

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($Arch -ne "x64") {
    exit $LASTEXITCODE
}

Write-Host "`n`n ===== Building large data file tests =====`n`n"

if ($BuildMethod -eq "cmake") {

    ./cxx/build-project.ps1 `
        -RepoName $RepoName `
        -ProjectDir $ProjectDir `
        -Name "$Name-LargeDataFiles" `
        -BuildDir "build-LargeDataFiles" `
        -Configuration $Configuration `
        -Arch $Arch `
        -BuildMethod $BuildMethod `
        -ExtraArgs @(
            "-DLargeDataFileSupport:BOOL=ON"
        )
        
} elseif ($BuildMethod -eq "msbuild") {

    Remove-Item -Recurse -Force "$RepoName/$ProjectDir/packages"

    ./cxx/build-project.ps1 `
        -RepoName $RepoName `
        -ProjectDir $ProjectDir `
        -Name "$Name-LargeDataFiles" `
        -BuildDir "build-LargeDataFiles" `
        -Configuration "$Configuration-LargeDataFiles" `
        -Arch $Arch `
        -BuildMethod $BuildMethod

} else {
    Write-Error "The build method '$BuildMethod' is not supported."
}

exit $LASTEXITCODE

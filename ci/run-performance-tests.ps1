param(
    [string]$RepoName,
    [string]$ProjectDir = ".",
    [string]$Name = "Release_x64",
    [string]$Arch = "x64",
    [string]$Configuration = "Release",
    [string]$BuildMethod = "cmake"
)

./c/run-performance-tests.ps1 -RepoName "common-cxx" -ProjectDir $ProjectDir -Name $Name -Configuration $Configuration -Arch $Arch


if ($BuildMethod -eq "cmake") {

    $RepoPath = [IO.Path]::Combine($pwd, $RepoName)
    $PerfResultsFile = [IO.Path]::Combine($RepoPath, "test-results", "performance-summary", "results_$Name.json")

    Push-Location $RepoPath
    try {
        if ($(Test-Path -Path "test-results") -eq  $False) {
            mkdir test-results
        }
    
        if ($(Test-Path -Path "test-results/performance-summary") -eq  $False) {
            mkdir test-results/performance-summary
        }
    
        $OutputFile = [IO.Path]::Combine($RepoPath, "summary.json")
        if ($IsWindows) {
            $PerfPath = [IO.Path]::Combine($RepoPath, "build", "bin", $Configuration, "CachePerf.exe")
        }
        else {
            $PerfPath = [IO.Path]::Combine($RepoPath, "build", "bin", "CachePerf")
        }
        . $PerfPath $OutputFile
        $Results = Get-Content $OutputFile | ConvertFrom-Json -AsHashtable
        Write-Output "{
            'HigherIsBetter': {
                'CacheFetchesPerSecond': $($Results.CacheFetchesPerSecond)
                'CacheFetchesPerSecondPerThread': $($Results.CacheFetchesPerSecondPerThread)
            },
            'LowerIsBetter': {
            }
        }" > $PerfResultsFile

    }
    finally {
        Pop-Location
    }

}

exit $LASTEXITCODE

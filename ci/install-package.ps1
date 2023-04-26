param (
    [string]$RepoName
)

$PackagePath = [IO.Path]::Combine($pwd, $RepoName, "package")
$BinPath = [IO.Path]::Combine($pwd, $RepoName, "build", "bin")

mkdir -Force $BinPath

Copy-Item -Recurse -Path $PackagePath -Destination $BinPath
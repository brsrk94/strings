$ErrorActionPreference = "Stop"

$appName = "strix"
$exeName = "strix.exe"
$installDir = "$env:LOCALAPPDATA\Programs\$appName"
$sourceExe = Join-Path $PSScriptRoot $exeName

# Check if source executable exists
if (-not (Test-Path $sourceExe)) {
    Write-Error "Error: '$exeName' not found in the current directory. Please compile it first or ensure you are running this script from the correct location."
    exit 1
}

# Create installation directory
if (-not (Test-Path $installDir)) {
    Write-Host "Creating installation directory: $installDir"
    New-Item -ItemType Directory -Force -Path $installDir | Out-Null
}

# Copy executable
Write-Host "Copying $exeName to $installDir..."
Copy-Item -Path $sourceExe -Destination "$installDir\$exeName" -Force

# Update User PATH
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($userPath -split ';' -notcontains $installDir) {
    Write-Host "Adding $installDir to User PATH..."
    $newPath = "$userPath;$installDir"
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "Success! $appName has been installed."
    Write-Host "NOTE: You must restart your PowerShell window or run 'refreshenv' to use the '$appName' command."
} else {
    Write-Host "Success! $appName is installed and already in your PATH."
}
